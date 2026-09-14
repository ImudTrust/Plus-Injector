#pragma once

#include <windows.h>
#include <psapi.h>
#include <string>
#include "Types.hpp"

namespace pi::native {
    using pi::ModuleInfo;
    using pi::ExportedFunction;

    HANDLE    OpenProcess(DWORD access, BOOL inherit, DWORD pid);
    BOOL      CloseHandleSafe(HANDLE h);
    BOOL      IsWow64ProcessSafe(HANDLE h, BOOL* wow64);
    BOOL      EnumProcessModulesEx(HANDLE h, HMODULE* mods, DWORD cb,
        DWORD* needed, DWORD filter);
    DWORD     GetModuleFileNameEx(HANDLE h, HMODULE mod,
        wchar_t* buf, DWORD size);
    BOOL      GetModuleInformation(HANDLE h, HMODULE mod, ModuleInfo* info);
    BOOL      WriteProcessMemory(HANDLE h, uintptr_t addr,
        const void* buf, SIZE_T size);
    BOOL      ReadProcessMemory(HANDLE h, uintptr_t addr,
        void* buf, SIZE_T size);
    uintptr_t VirtualAllocEx(HANDLE h, SIZE_T size,
        DWORD allocType, DWORD protect);
    BOOL      VirtualFreeEx(HANDLE h, uintptr_t addr,
        SIZE_T size, DWORD freeType);
    BOOL      FlushInstructionCache(HANDLE h, uintptr_t addr, SIZE_T size);
    HANDLE    CreateRemoteThread(HANDLE h, uintptr_t start, uintptr_t param);
    DWORD     WaitForSingleObject(HANDLE h, DWORD ms);

}
