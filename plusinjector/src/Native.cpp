#include "header/Native.hpp"

namespace pi::native {

    HANDLE OpenProcess(DWORD access, BOOL inherit, DWORD pid) {
        return ::OpenProcess(access, inherit, pid);
    }

    BOOL CloseHandleSafe(HANDLE h) {
        return ::CloseHandle(h);
    }

    BOOL IsWow64ProcessSafe(HANDLE h, BOOL* wow64) {
        return ::IsWow64Process(h, wow64);
    }

    BOOL EnumProcessModulesEx(HANDLE h, HMODULE* mods, DWORD cb,
        DWORD* needed, DWORD filter) {
        return ::EnumProcessModulesEx(h, mods, cb, needed, filter);
    }

    DWORD GetModuleFileNameEx(HANDLE h, HMODULE mod, wchar_t* buf, DWORD size) {
        return ::GetModuleFileNameExW(h, mod, buf, size);
    }

    BOOL GetModuleInformation(HANDLE h, HMODULE mod, ModuleInfo* info) {
        MODULEINFO mi{};
        BOOL ok = ::GetModuleInformation(h, mod, &mi, sizeof(mi));
        if (ok) {
            info->lpBaseOfDll = reinterpret_cast<uintptr_t>(mi.lpBaseOfDll);
            info->SizeOfImage = mi.SizeOfImage;
            info->EntryPoint = reinterpret_cast<uintptr_t>(mi.EntryPoint);
        }
        return ok;
    }

    BOOL WriteProcessMemory(HANDLE h, uintptr_t addr,
        const void* buf, SIZE_T size) {
        SIZE_T written = 0;
        return ::WriteProcessMemory(h, reinterpret_cast<LPVOID>(addr),
            buf, size, &written) && written == size;
    }

    BOOL ReadProcessMemory(HANDLE h, uintptr_t addr,
        void* buf, SIZE_T size) {
        SIZE_T read = 0;
        return ::ReadProcessMemory(h, reinterpret_cast<LPCVOID>(addr),
            buf, size, &read) && read == size;
    }

    uintptr_t VirtualAllocEx(HANDLE h, SIZE_T size,
        DWORD allocType, DWORD protect) {
        return reinterpret_cast<uintptr_t>(
            ::VirtualAllocEx(h, nullptr, size, allocType, protect));
    }

    BOOL VirtualFreeEx(HANDLE h, uintptr_t addr,
        SIZE_T size, DWORD freeType) {
        return ::VirtualFreeEx(h, reinterpret_cast<LPVOID>(addr),
            size, freeType);
    }

    BOOL FlushInstructionCache(HANDLE h, uintptr_t addr, SIZE_T size) {
        return ::FlushInstructionCache(h, reinterpret_cast<LPCVOID>(addr), size);
    }

    HANDLE CreateRemoteThread(HANDLE h, uintptr_t start, uintptr_t param) {
        return ::CreateRemoteThread(h, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(start),
            reinterpret_cast<LPVOID>(param), 0, nullptr);
    }

    DWORD WaitForSingleObject(HANDLE h, DWORD ms) {
        return ::WaitForSingleObject(h, ms);
    }

}
