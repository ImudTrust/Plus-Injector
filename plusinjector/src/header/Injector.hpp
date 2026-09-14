#pragma once

#include <windows.h>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pi {

    class Memory;

    class Injector {
    public:
        explicit Injector(const std::string& processName);
        explicit Injector(uint32_t processId);
        Injector(HANDLE handle, uintptr_t monoModule);
        ~Injector();

        bool Is64Bit() const { return _is64Bit; }

        uintptr_t Inject(const std::vector<uint8_t>& rawAssembly,
            const std::string& ns,
            const std::string& className,
            const std::string& methodName);

        void Eject(uintptr_t assembly,
            const std::string& ns,
            const std::string& className,
            const std::string& methodName);

    private:
        void ObtainMonoExports();

        uintptr_t GetRootDomain();
        uintptr_t OpenImageFromData(const std::vector<uint8_t>& assembly);
        uintptr_t OpenAssemblyFromImage(uintptr_t image);
        uintptr_t GetImageFromAssembly(uintptr_t assembly);
        uintptr_t GetClassFromName(uintptr_t image,
            const std::string& ns,
            const std::string& name);
        uintptr_t GetMethodFromName(uintptr_t klass, const std::string& name);
        std::string GetClassName(uintptr_t monoObject);
        std::string ReadMonoString(uintptr_t monoString);
        void        RuntimeInvoke(uintptr_t method);
        void        CloseAssembly(uintptr_t assembly);

        uintptr_t Execute(uintptr_t address,
            const std::vector<uintptr_t>& args);

        std::vector<uint8_t> Assemble(uintptr_t fn, uintptr_t retValPtr,
            const std::vector<uintptr_t>& args);
        std::vector<uint8_t> Assemble64(uintptr_t fn, uintptr_t retValPtr,
            const std::vector<uintptr_t>& args);
        std::vector<uint8_t> Assemble86(uintptr_t fn, uintptr_t retValPtr,
            const std::vector<uintptr_t>& args);

        HANDLE      _handle = nullptr;
        uintptr_t   _mono = 0;
        bool        _is64Bit = true;
        bool        _attach = false;
        uintptr_t   _rootDomain = 0;

        std::unique_ptr<Memory> _memory;
        std::unordered_map<std::string, uintptr_t> _exports;
    };

}