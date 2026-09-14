#include "header/ProcessUtils.hpp"
#include "header/Memory.hpp"
#include "header/Native.hpp"
#include "header/Exceptions.hpp"
#include <algorithm>
#include <vector>

namespace pi {

    std::vector<ExportedFunction> ProcessUtils::GetExportedFunctions(
        HANDLE h, uintptr_t mod) {

        std::vector<ExportedFunction> out;
        Memory mem(h);

        int32_t e_lfanew = mem.ReadInt(mod + 0x3C);
        uintptr_t ntHeaders = mod + e_lfanew;
        uintptr_t optionalHeader = ntHeaders + 0x18;
        uintptr_t dataDirectory = optionalHeader +
            (Is64BitProcess(h) ? 0x70 : 0x60);

        uintptr_t exportDirectory = mod + mem.ReadInt(dataDirectory);
        uintptr_t names = mod + mem.ReadInt(exportDirectory + 0x20);
        uintptr_t ordinals = mod + mem.ReadInt(exportDirectory + 0x24);
        uintptr_t functions = mod + mem.ReadInt(exportDirectory + 0x1C);
        int32_t   count = mem.ReadInt(exportDirectory + 0x18);

        for (int i = 0; i < count; ++i) {
            int32_t offset = mem.ReadInt(names + i * 4);
            std::string name = mem.ReadString(mod + offset, 32);
            int16_t ordinal = mem.ReadShort(ordinals + i * 2);
            uintptr_t address = mod + mem.ReadInt(functions + ordinal * 4);
            if (address != 0) {
                out.emplace_back(std::move(name), address);
            }
        }
        return out;
    }

    bool ProcessUtils::GetMonoModule(HANDLE h, uintptr_t& monoModule) {
        DWORD size = Is64BitProcess(h) ? 8 : 4;

        DWORD needed = 0;
        if (!native::EnumProcessModulesEx(
            h, nullptr, 0, &needed, LIST_MODULES_ALL)) {
            throw PlusInjectorException(
                "Failed to enumerate process modules", GetLastError());
        }

        DWORD count = needed / size;
        std::vector<HMODULE> mods(count, nullptr);

        if (!native::EnumProcessModulesEx(
            h, mods.data(), needed, &needed, LIST_MODULES_ALL)) {
            throw PlusInjectorException(
                "Failed to enumerate process modules", GetLastError());
        }

        for (DWORD i = 0; i < count; ++i) {
            wchar_t path[MAX_PATH]{};
            native::GetModuleFileNameEx(h, mods[i], path, MAX_PATH);

            std::wstring lower(path);
            std::transform(lower.begin(), lower.end(),
                lower.begin(), ::towlower);

            if (lower.find(L"mono") != std::wstring::npos) {
                ModuleInfo info{};
                if (!native::GetModuleInformation(h, mods[i], &info)) {
                    throw PlusInjectorException(
                        "Failed to get module information", GetLastError());
                }

                auto funcs = GetExportedFunctions(h, info.lpBaseOfDll);
                for (auto& f : funcs) {
                    if (f.Name == "mono_get_root_domain") {
                        monoModule = info.lpBaseOfDll;
                        return true;
                    }
                }
            }
        }
        monoModule = 0;
        return false;
    }

    bool ProcessUtils::Is64BitProcess(HANDLE h) {
        SYSTEM_INFO si{};
        GetNativeSystemInfo(&si);

        if (si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64 &&
            si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_IA64) {
            return false;
        }

        BOOL wow64 = FALSE;
        if (!native::IsWow64ProcessSafe(h, &wow64)) {
            return sizeof(void*) == 8;
        }
        return !wow64;
    }

}