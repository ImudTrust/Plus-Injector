#pragma once

#include <windows.h>
#include <cstdint>
#include <string>

namespace pi 
{

    struct ModuleInfo {
        uintptr_t lpBaseOfDll = 0;
        uint32_t  SizeOfImage = 0;
        uintptr_t EntryPoint = 0;
    };

    struct ExportedFunction {
        std::string Name;
        uintptr_t   Address = 0;

        ExportedFunction() = default;
        ExportedFunction(std::string n, uintptr_t a)
            : Name(std::move(n)), Address(a) {
        }
    };

    enum class MonoImageOpenStatus : int32_t {
        MONO_IMAGE_OK = 0,
        MONO_IMAGE_ERROR_ERRNO,
        MONO_IMAGE_MISSING_ASSEMBLYREF,
        MONO_IMAGE_IMAGE_INVALID
    };

    constexpr DWORD PI_PROCESS_ALL_ACCESS =
        PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD | PROCESS_DUP_HANDLE |
        PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION | PROCESS_SET_QUOTA |
        PROCESS_SUSPEND_RESUME | PROCESS_TERMINATE | PROCESS_VM_OPERATION |
        PROCESS_VM_READ | PROCESS_VM_WRITE | SYNCHRONIZE | 0x1000;

}