#include "header/Injector.hpp"
#include "header/Memory.hpp"
#include "header/Assembler.hpp"
#include "header/ProcessUtils.hpp"
#include "header/Native.hpp"
#include "header/Exceptions.hpp"

#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>
#include <memory>

namespace pi {

    static const char* k_get_root_domain = "mono_get_root_domain";
    static const char* k_thread_attach = "mono_thread_attach";
    static const char* k_image_open_from_data = "mono_image_open_from_data";
    static const char* k_assembly_load_from_full = "mono_assembly_load_from_full";
    static const char* k_assembly_get_image = "mono_assembly_get_image";
    static const char* k_class_from_name = "mono_class_from_name";
    static const char* k_class_get_method_from_name = "mono_class_get_method_from_name";
    static const char* k_runtime_invoke = "mono_runtime_invoke";
    static const char* k_assembly_close = "mono_assembly_close";
    static const char* k_image_strerror = "mono_image_strerror";
    static const char* k_object_get_class = "mono_object_get_class";
    static const char* k_class_get_name = "mono_class_get_name";

    Injector::Injector(const std::string& processName) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) {
            throw PlusInjectorException(
                "CreateToolhelp32Snapshot failed", GetLastError());
        }

        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);

        DWORD foundPid = 0;
        std::wstring wname(processName.begin(), processName.end());
        std::wstring lowerTarget = wname;
        std::transform(lowerTarget.begin(), lowerTarget.end(),
            lowerTarget.begin(), ::towlower);

        if (Process32FirstW(snap, &pe)) {
            do {
                std::wstring cur = pe.szExeFile;
                std::wstring lower = cur;
                std::transform(lower.begin(), lower.end(),
                    lower.begin(), ::towlower);

                if (lower == lowerTarget ||
                    lower == lowerTarget + L".exe") {
                    foundPid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);

        if (foundPid == 0) {
            throw PlusInjectorException(
                "Could not find a process with the name " + processName);
        }

        _handle = native::OpenProcess(PI_PROCESS_ALL_ACCESS, FALSE, foundPid);
        if (_handle == nullptr) {
            throw PlusInjectorException(
                "Failed to open process", GetLastError());
        }

        _is64Bit = ProcessUtils::Is64BitProcess(_handle);
        if (!ProcessUtils::GetMonoModule(_handle, _mono)) {
            throw PlusInjectorException(
                "Failed to find mono.dll in the target process");
        }

        _memory = std::make_unique<Memory>(_handle);
    }

    Injector::Injector(uint32_t processId) {
        _handle = native::OpenProcess(PI_PROCESS_ALL_ACCESS, FALSE, processId);
        if (_handle == nullptr) {
            throw PlusInjectorException(
                "Failed to open process", GetLastError());
        }

        _is64Bit = ProcessUtils::Is64BitProcess(_handle);
        if (!ProcessUtils::GetMonoModule(_handle, _mono)) {
            throw PlusInjectorException(
                "Failed to find mono.dll in the target process");
        }

        _memory = std::make_unique<Memory>(_handle);
    }

    Injector::Injector(HANDLE h, uintptr_t monoModule) {
        if (h == nullptr) {
            throw PlusInjectorException("Argument cannot be zero (handle)");
        }
        if (monoModule == 0) {
            throw PlusInjectorException("Argument cannot be zero (monoModule)");
        }
        _handle = h;
        _mono = monoModule;
        _is64Bit = ProcessUtils::Is64BitProcess(_handle);
        _memory = std::make_unique<Memory>(_handle);
    }

    Injector::~Injector() {
        _memory.reset();
        if (_handle) CloseHandle(_handle);
    }

    uintptr_t Injector::Inject(const std::vector<uint8_t>& rawAssembly,
        const std::string& ns,
        const std::string& className,
        const std::string& methodName) {
        if (rawAssembly.empty()) {
            throw PlusInjectorException("rawAssembly cannot be empty");
        }

        ObtainMonoExports();
        _rootDomain = GetRootDomain();
        uintptr_t rawImage = OpenImageFromData(rawAssembly);
        _attach = true;
        uintptr_t assembly = OpenAssemblyFromImage(rawImage);
        uintptr_t image = GetImageFromAssembly(assembly);
        uintptr_t klass = GetClassFromName(image, ns, className);
        uintptr_t method = GetMethodFromName(klass, methodName);
        RuntimeInvoke(method);
        return assembly;
    }

    void Injector::Eject(uintptr_t assembly,
        const std::string& ns,
        const std::string& className,
        const std::string& methodName) {
        if (assembly == 0) {
            throw PlusInjectorException("assembly cannot be zero");
        }

        ObtainMonoExports();
        _rootDomain = GetRootDomain();
        _attach = true;
        uintptr_t image = GetImageFromAssembly(assembly);
        uintptr_t klass = GetClassFromName(image, ns, className);
        uintptr_t method = GetMethodFromName(klass, methodName);
        RuntimeInvoke(method);
        CloseAssembly(assembly);
    }

    static void ThrowIfNull(uintptr_t ptr, const char* method) {
        if (ptr == 0) {
            throw PlusInjectorException(
                std::string(method) + "() returned NULL");
        }
    }

    void Injector::ObtainMonoExports() {
        const char* names[] = {
            k_get_root_domain, k_thread_attach,
            k_image_open_from_data, k_assembly_load_from_full,
            k_assembly_get_image, k_class_from_name,
            k_class_get_method_from_name, k_runtime_invoke,
            k_assembly_close, k_image_strerror,
            k_object_get_class, k_class_get_name
        };

        for (auto* n : names) _exports[n] = 0;

        for (auto& ef : ProcessUtils::GetExportedFunctions(_handle, _mono)) {
            auto it = _exports.find(ef.Name);
            if (it != _exports.end()) it->second = ef.Address;
        }

        for (auto& kv : _exports) {
            if (kv.second == 0) {
                throw PlusInjectorException(
                    "Failed to obtain the address of " + kv.first + "()");
            }
        }
    }

    uintptr_t Injector::GetRootDomain() {
        uintptr_t rd = Execute(_exports[k_get_root_domain], {});
        ThrowIfNull(rd, k_get_root_domain);
        return rd;
    }

    uintptr_t Injector::OpenImageFromData(
        const std::vector<uint8_t>& assembly) {

        uintptr_t statusPtr = _memory->Allocate(4);
        uintptr_t dataPtr = _memory->AllocateAndWrite(assembly);

        uintptr_t rawImage = Execute(_exports[k_image_open_from_data], {
            dataPtr,
            (uintptr_t)assembly.size(),
            (uintptr_t)1,
            statusPtr
            });

        MonoImageOpenStatus status =
            (MonoImageOpenStatus)_memory->ReadInt(statusPtr);

        if (status != MonoImageOpenStatus::MONO_IMAGE_OK) {
            uintptr_t msgPtr = Execute(_exports[k_image_strerror],
                { (uintptr_t)status });
            std::string msg = _memory->ReadString(msgPtr, 256);
            throw PlusInjectorException(
                std::string(k_image_open_from_data) + "() failed: " + msg);
        }
        return rawImage;
    }

    uintptr_t Injector::OpenAssemblyFromImage(uintptr_t image) {
        uintptr_t statusPtr = _memory->Allocate(4);

        std::vector<uint8_t> nameBytes{ 0 };
        uintptr_t namePtr = _memory->AllocateAndWrite(nameBytes);

        uintptr_t assembly = Execute(_exports[k_assembly_load_from_full], {
            image, namePtr, statusPtr, (uintptr_t)0
            });

        MonoImageOpenStatus status =
            (MonoImageOpenStatus)_memory->ReadInt(statusPtr);

        if (status != MonoImageOpenStatus::MONO_IMAGE_OK) {
            uintptr_t msgPtr = Execute(_exports[k_image_strerror],
                { (uintptr_t)status });
            std::string msg = _memory->ReadString(msgPtr, 256);
            throw PlusInjectorException(
                std::string(k_assembly_load_from_full) + "() failed: " + msg);
        }
        return assembly;
    }

    uintptr_t Injector::GetImageFromAssembly(uintptr_t assembly) {
        uintptr_t image = Execute(_exports[k_assembly_get_image], { assembly });
        ThrowIfNull(image, k_assembly_get_image);
        return image;
    }

    uintptr_t Injector::GetClassFromName(uintptr_t image,
        const std::string& ns,
        const std::string& name) {
        uintptr_t nsPtr = _memory->AllocateAndWrite(ns);
        uintptr_t nPtr = _memory->AllocateAndWrite(name);

        uintptr_t klass = Execute(_exports[k_class_from_name],
            { image, nsPtr, nPtr });
        ThrowIfNull(klass, k_class_from_name);
        return klass;
    }

    uintptr_t Injector::GetMethodFromName(uintptr_t klass,
        const std::string& name) {
        uintptr_t nPtr = _memory->AllocateAndWrite(name);

        uintptr_t method = Execute(_exports[k_class_get_method_from_name],
            { klass, nPtr, (uintptr_t)0 });
        ThrowIfNull(method, k_class_get_method_from_name);
        return method;
    }

    std::string Injector::GetClassName(uintptr_t monoObject) {
        uintptr_t klass = Execute(_exports[k_object_get_class], { monoObject });
        ThrowIfNull(klass, k_object_get_class);

        uintptr_t namePtr = Execute(_exports[k_class_get_name], { klass });
        ThrowIfNull(namePtr, k_class_get_name);

        return _memory->ReadString(namePtr, 256);
    }

    std::string Injector::ReadMonoString(uintptr_t monoString) {
        if (monoString == 0) return {};

        uintptr_t lenOffset = _is64Bit ? 0x10 : 0x8;
        uintptr_t strOffset = _is64Bit ? 0x14 : 0xC;
        int32_t len = _memory->ReadInt(monoString + lenOffset);
        constexpr int32_t kMaxMessageLength = 1024 * 1024;
        if (len <= 0) return {};
        if (len > kMaxMessageLength) return "(managed exception message is too large)";
        return _memory->ReadUnicodeString(monoString + strOffset, len * 2);
    }

    void Injector::RuntimeInvoke(uintptr_t method) {
        uintptr_t excPtr = _memory->AllocateAndWrite((int64_t)0);

        Execute(_exports[k_runtime_invoke], {
            method,
            (uintptr_t)0,
            (uintptr_t)0,
            excPtr
            });

        uintptr_t exc = (uintptr_t)_memory->ReadLong(excPtr);

        if (exc != 0) {
            std::string className = GetClassName(exc);
            uintptr_t msgFieldOffset = _is64Bit ? 0x20 : 0x10;
            uintptr_t msgPtr = (uintptr_t)_memory->ReadLong(exc + msgFieldOffset);
            std::string message = ReadMonoString(msgPtr);
            if (message.empty()) message = "(no message)";
            throw PlusInjectorException(
                "The managed method threw an exception: (" +
                className + ") " + message);
        }
    }

    void Injector::CloseAssembly(uintptr_t assembly) {
        // mono_assembly_close returns nothing, so don't use its return value as a pointer.
        Execute(_exports[k_assembly_close], { assembly });
    }

    uintptr_t Injector::Execute(uintptr_t address,
        const std::vector<uintptr_t>& args) {
        uintptr_t retValPtr = _memory->AllocateAndWrite((int64_t)0);

        std::vector<uint8_t> code = Assemble(address, retValPtr, args);
        uintptr_t alloc = _memory->AllocateAndWrite(code);

        if (!native::FlushInstructionCache(_handle, alloc, code.size())) {
            throw PlusInjectorException(
                "Failed to flush the remote instruction cache", GetLastError());
        }

        HANDLE thread = native::CreateRemoteThread(_handle, alloc, 0);
        if (thread == nullptr) {
            throw PlusInjectorException(
                "Failed to create a remote thread", GetLastError());
        }

        DWORD wait = native::WaitForSingleObject(thread, INFINITE);
        if (wait != WAIT_OBJECT_0) {
            CloseHandle(thread);
            throw PlusInjectorException(
                "Failed to wait for a remote thread", GetLastError());
        }
        CloseHandle(thread);

        uintptr_t ret = _is64Bit
            ? (uintptr_t)_memory->ReadLong(retValPtr)
            : (uintptr_t)_memory->ReadInt(retValPtr);

        if (_is64Bit) {
            if (ret == 0x00000000C0000005ULL) {
                throw PlusInjectorException(
                    "An access violation occurred while executing the function");
            }
        }
        else {
            if ((ret & 0xFFFFFFFF) == 0xC0000005) {
                throw PlusInjectorException(
                    "An access violation occurred while executing the function");
            }
        }

        return ret;
    }

    std::vector<uint8_t> Injector::Assemble(uintptr_t fn, uintptr_t retValPtr,
        const std::vector<uintptr_t>& args) {
        return _is64Bit ? Assemble64(fn, retValPtr, args)
            : Assemble86(fn, retValPtr, args);
    }

    std::vector<uint8_t> Injector::Assemble86(uintptr_t functionPtr,
        uintptr_t retValPtr,
        const std::vector<uintptr_t>& args) {
        Assembler asm_;

        if (_attach) {
            asm_.Push(_rootDomain);
            asm_.MovEax((uint32_t)_exports[k_thread_attach]);
            asm_.CallEax();
            asm_.AddEsp(4);
        }

        for (auto it = args.rbegin(); it != args.rend(); ++it) {
            asm_.Push(*it);
        }

        asm_.MovEax((uint32_t)functionPtr);
        asm_.CallEax();
        asm_.AddEsp((uint8_t)(args.size() * 4));
        asm_.MovEaxTo(retValPtr);
        asm_.Return();

        return asm_.ToByteArray();
    }

    std::vector<uint8_t> Injector::Assemble64(uintptr_t functionPtr,
        uintptr_t retValPtr,
        const std::vector<uintptr_t>& args) {
        Assembler asm_;

        asm_.SubRsp(40);

        if (_attach) {
            asm_.MovRax(_exports[k_thread_attach]);
            asm_.MovRcx(_rootDomain);
            asm_.CallRax();
        }

        asm_.MovRax(functionPtr);

        for (size_t i = 0; i < args.size() && i < 4; ++i) {
            switch (i) {
            case 0: asm_.MovRcx(args[i]); break;
            case 1: asm_.MovRdx(args[i]); break;
            case 2: asm_.MovR8(args[i]); break;
            case 3: asm_.MovR9(args[i]); break;
            }
        }

        asm_.CallRax();
        asm_.AddRsp(40);
        asm_.MovRaxTo(retValPtr);
        asm_.Return();

        return asm_.ToByteArray();
    }

}
