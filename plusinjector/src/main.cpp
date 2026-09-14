#include "header/Injector.hpp"
#include "header/Args.hpp"
#include "header/Exceptions.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <memory>
#include <vector>
#include <charconv>
#include <chrono>
#include <limits>
#include <thread>
#include <utility>
#include <windows.h>

namespace pi {

    static void PrintHelp() {
        std::cout <<
            "PlusInjector 1.0\r\n\r\n"
            "Usage:\r\n"
            "pi.exe <inject|eject> <options>\r\n\r\n"
            "Options:\r\n"
            "-p, --process <name|pid> - Target process\r\n"
            "-a, --assembly <path|address> - When injecting, the path of the assembly to inject. "
            "When ejecting, the address of the assembly to eject\r\n"
            "-n, --namespace <name> - Loader namespace (optional)\r\n"
            "-c, --class <name> - Loader class name\r\n"
            "-m, --method <name> - Zero-argument method to invoke\r\n"
            "-d, --delay <time> - Delay before the operation: e.g. 10ms, 10s, 10m\r\n"
            "-h, --hide - Hide the console after startup\r\n"
            "-log, --log <path> - Write console output to a log file\r\n"
            "-out, --result <path> - Write the injected assembly address to a file\r\n"
            "--help - Show this help\r\n\r\n"
            "Examples:\r\n"
            "pi.exe inject -p testgame -a ExampleAssembly.dll "
            "-n ExampleAssembly -c Loader -m Load -d 10s\r\n"
            "pi.exe eject -p testgame -a 0x13D23A98 "
            "-n ExampleAssembly -c Loader -m Unload\r\n";
    }

    static bool IsPresent(const CommandLineArguments& cla,
        const std::string& shortName, const std::string& longName) {
        return cla.IsSwitchPresent(shortName) || cla.IsSwitchPresent(longName);
    }

    static bool GetString(const CommandLineArguments& cla,
        const std::string& shortName, const std::string& longName,
        std::string& value) {
        return cla.GetStringArg(shortName, value) || cla.GetStringArg(longName, value);
    }

    static bool HasAliasConflict(const CommandLineArguments& cla,
        const std::string& shortName, const std::string& longName) {
        return cla.IsSwitchPresent(shortName) && cla.IsSwitchPresent(longName);
    }

    class TeeStreamBuffer final : public std::streambuf {
    public:
        TeeStreamBuffer(std::streambuf* console, std::streambuf* file)
            : _console(console), _file(file) {}

    protected:
        int overflow(int c) override {
            if (c == EOF) return !EOF;
            const char ch = static_cast<char>(c);
            return (_console->sputc(ch) == EOF || _file->sputc(ch) == EOF) ? EOF : c;
        }
        int sync() override {
            return (_console->pubsync() == 0 && _file->pubsync() == 0) ? 0 : -1;
        }
    private:
        std::streambuf* _console;
        std::streambuf* _file;
    };

    class CoutRedirect final {
    public:
        explicit CoutRedirect(std::streambuf* replacement)
            : _original(std::cout.rdbuf(replacement)) {}
        ~CoutRedirect() { std::cout.rdbuf(_original); }

        CoutRedirect(const CoutRedirect&) = delete;
        CoutRedirect& operator=(const CoutRedirect&) = delete;
    private:
        std::streambuf* _original;
    };

    static bool ParseDelay(const CommandLineArguments& cla,
        std::chrono::milliseconds& delay, std::string& error) {
        delay = std::chrono::milliseconds::zero();

        if (!IsPresent(cla, "-d", "--delay")) return true;

        std::string value;
        if (!GetString(cla, "-d", "--delay", value)) {
            error = "Delay must be a number followed by ms, s, or m (for example: -d 10s)";
            return false;
        }

        int64_t multiplier = 0;
        size_t suffixLength = 0;
        if (value.size() > 2 && value.compare(value.size() - 2, 2, "ms") == 0) {
            multiplier = 1;
            suffixLength = 2;
        }
        else if (value.size() > 1 && value.back() == 's') {
            multiplier = 1000;
            suffixLength = 1;
        }
        else if (value.size() > 1 && value.back() == 'm') {
            multiplier = 60 * 1000;
            suffixLength = 1;
        }
        else {
            error = "Invalid delay '" + value + "'. Use ms, s, or m (for example: 10ms, 10s, 10m)";
            return false;
        }

        const std::string number = value.substr(0, value.size() - suffixLength);
        int64_t amount = 0;
        const auto result = std::from_chars(number.data(),
            number.data() + number.size(), amount);
        if (result.ec != std::errc() || result.ptr != number.data() + number.size() || amount < 0 ||
            amount > (std::numeric_limits<int64_t>::max)() / multiplier) {
            error = "Invalid delay '" + value + "'. The value must be a non-negative whole number.";
            return false;
        }

        delay = std::chrono::milliseconds(amount * multiplier);
        return true;
    }

    static std::vector<uint8_t> ReadFileBytes(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            throw PlusInjectorException("Could not read the file " + path);
        }
        return std::vector<uint8_t>(
            std::istreambuf_iterator<char>(f),
            std::istreambuf_iterator<char>());
    }

    static bool Inject(Injector& inj, const CommandLineArguments& cla,
        std::ostream* resultFile) {
        std::string assemblyPath, ns, className, methodName;

        if (!GetString(cla, "-a", "--assembly", assemblyPath)) {
            std::cout << "No assembly specified\n";
            return false;
        }

        std::vector<uint8_t> assembly;
        try {
            assembly = ReadFileBytes(assemblyPath);
        }
        catch (const std::exception& e) {
            std::cout << e.what() << "\n";
            return false;
        }

        GetString(cla, "-n", "--namespace", ns);

        if (!GetString(cla, "-c", "--class", className)) {
            std::cout << "No class name specified\n";
            return false;
        }
        if (!GetString(cla, "-m", "--method", methodName)) {
            std::cout << "No method name specified\n";
            return false;
        }

        uintptr_t remoteAssembly = 0;
        try {
            remoteAssembly = inj.Inject(assembly, ns, className, methodName);
        }
        catch (const PlusInjectorException& ie) {
            std::cout << "Failed to inject assembly: " << ie.what() << "\n";
            return false;
        }
        catch (const std::exception& exc) {
            std::cout << "Failed to inject assembly (unknown error): "
                << exc.what() << "\n";
            return false;
        }

        if (remoteAssembly == 0) return false;

        std::string fname =
            std::filesystem::path(assemblyPath).filename().string();

        std::ostringstream address;
        if (inj.Is64Bit()) {
            address << "0x" << std::hex << std::uppercase
                << std::setw(16) << std::setfill('0')
                << (uint64_t)remoteAssembly;
        }
        else {
            address << "0x" << std::hex << std::uppercase
                << std::setw(8) << std::setfill('0')
                << (uint32_t)remoteAssembly;
        }
        std::cout << fname << ": " << address.str() << "\n";
        if (resultFile != nullptr) {
            *resultFile << address.str() << "\n";
            if (!*resultFile) {
                std::cout << "Injected, but failed to write the result file\n";
                return false;
            }
        }
        return true;
    }

    static bool Eject(Injector& inj, const CommandLineArguments& cla) {
        uintptr_t assembly = 0;
        int intPtr = 0;
        long long longPtr = 0;

        if (cla.GetIntArg("-a", intPtr) || cla.GetIntArg("--assembly", intPtr)) {
            assembly = (uintptr_t)(uint32_t)intPtr;
        }
        else if (cla.GetLongArg("-a", longPtr) || cla.GetLongArg("--assembly", longPtr)) {
            assembly = (uintptr_t)longPtr;
        }
        else {
            std::cout << "No assembly pointer specified\n";
            return false;
        }

        std::string ns, className, methodName;
        GetString(cla, "-n", "--namespace", ns);

        if (!GetString(cla, "-c", "--class", className)) {
            std::cout << "No class name specified\n";
            return false;
        }
        if (!GetString(cla, "-m", "--method", methodName)) {
            std::cout << "No method name specified\n";
            return false;
        }

        try {
            inj.Eject(assembly, ns, className, methodName);
            std::cout << "Ejection successful\n";
            return true;
        }
        catch (const PlusInjectorException& ie) {
            std::cout << "Ejection failed: " << ie.what() << "\n";
            return false;
        }
        catch (const std::exception& exc) {
            std::cout << "Ejection failed (unknown error): "
                << exc.what() << "\n";
            return false;
        }
    }

}

int main(int argc, char** argv) {
    using namespace pi;

    if (argc == 1 || std::string(argv[1]) == "--help" ||
        std::string(argv[1]) == "help") {
        PrintHelp();
        return argc == 1 ? 2 : 0;
    }

    // The operation is the first argument, like `pi.exe inject`.
    // CommandLineArguments only stores options starting with `-`, so "inject" won't be found there.
    const std::string operation = argv[1];
    const bool inject = operation == "inject";
    const bool eject = operation == "eject";

    if (!inject && !eject) {
        std::cout << "Invalid operation. Specify inject or eject\n";
        PrintHelp();
        return 2;
    }

    CommandLineArguments cla(argc, argv);

    if (cla.IsSwitchPresent("--help")) {
        PrintHelp();
        return 0;
    }

    const std::vector<std::string> allowed{
        "-p", "--process", "-a", "--assembly", "-n", "--namespace",
        "-c", "--class", "-m", "--method", "-d", "--delay",
        "-h", "--hide", "-log", "--log", "-out", "--result", "--help"
    };
    const auto unknown = cla.GetUnknownSwitches(allowed);
    const auto duplicates = cla.GetDuplicateSwitches();
    if (!unknown.empty() || !duplicates.empty()) {
        if (!unknown.empty()) std::cout << "Unknown option: " << unknown.front() << "\n";
        else std::cout << "Duplicate option: " << duplicates.front() << "\n";
        return 2;
    }
    for (const auto& pair : { std::pair{"-p", "--process"}, std::pair{"-a", "--assembly"},
            std::pair{"-n", "--namespace"}, std::pair{"-c", "--class"},
            std::pair{"-m", "--method"}, std::pair{"-d", "--delay"},
            std::pair{"-h", "--hide"}, std::pair{"-log", "--log"},
            std::pair{"-out", "--result"} }) {
        if (HasAliasConflict(cla, pair.first, pair.second)) {
            std::cout << "Use either " << pair.first << " or " << pair.second << ", not both\n";
            return 2;
        }
    }
    if ((cla.IsSwitchPresent("-h") && cla.HasValue("-h")) ||
        (cla.IsSwitchPresent("--hide") && cla.HasValue("--hide"))) {
        std::cout << "-h/--hide does not accept a value\n";
        return 2;
    }

    std::ofstream logFile;
    std::ofstream resultFile;
    std::unique_ptr<TeeStreamBuffer> logBuffer;
    std::unique_ptr<CoutRedirect> coutRedirect;
    std::streambuf* originalCout = std::cout.rdbuf();
    std::string logPath;
    if (GetString(cla, "-log", "--log", logPath)) {
        logFile.open(logPath, std::ios::out | std::ios::trunc);
        if (!logFile) {
            std::cerr << "Could not open log file " << logPath << "\n";
            return 2;
        }
        logBuffer = std::make_unique<TeeStreamBuffer>(originalCout, logFile.rdbuf());
        coutRedirect = std::make_unique<CoutRedirect>(logBuffer.get());
    }
    std::string resultPath;
    if (IsPresent(cla, "-out", "--result")) {
        if (!inject) {
            std::cout << "-out/--result is only available for inject\n";
            return 2;
        }
        if (!GetString(cla, "-out", "--result", resultPath)) {
            std::cout << "-out/--result requires a file path\n";
            return 2;
        }
        resultFile.open(resultPath, std::ios::out | std::ios::trunc);
        if (!resultFile) {
            std::cout << "Could not open result file " << resultPath << "\n";
            return 2;
        }
    }
    if (IsPresent(cla, "-h", "--hide")) {
        if (HWND console = GetConsoleWindow()) ShowWindow(console, SW_HIDE);
    }

    std::chrono::milliseconds delay;
    std::string delayError;
    if (!ParseDelay(cla, delay, delayError)) {
        std::cout << delayError << "\n";
        return 2;
    }

    try {
        int pid = 0;
        std::string pname;
        std::unique_ptr<Injector> injector;

        if (cla.GetIntArg("-p", pid) || cla.GetIntArg("--process", pid)) {
            if (pid <= 0) {
                std::cout << "Process id must be a positive number\n";
                return 2;
            }
            injector = std::make_unique<Injector>((uint32_t)pid);
        }
        else if (GetString(cla, "-p", "--process", pname)) {
            injector = std::make_unique<Injector>(pname);
        }
        else {
            std::cout << "No process id/name specified\n";
            return 2;
        }

        if (delay.count() > 0) {
            std::cout << "Waiting " << delay.count() << " ms before "
                << operation << "...\n";
            std::this_thread::sleep_for(delay);
        }

        const bool succeeded = inject
            ? Inject(*injector, cla, resultFile.is_open() ? &resultFile : nullptr)
            : Eject(*injector, cla);
        return succeeded ? 0 : 1;
    }
    catch (const PlusInjectorException& e) {
        std::cout << e.what() << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cout << "Unknown error: " << e.what() << "\n";
        return 1;
    }
}
