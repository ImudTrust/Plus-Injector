#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include "Types.hpp"

namespace pi {

    class ProcessUtils {
    public:
        static std::vector<ExportedFunction> GetExportedFunctions(
            HANDLE h, uintptr_t mod);

        static bool GetMonoModule(HANDLE h, uintptr_t& monoModule);

        static bool Is64BitProcess(HANDLE h);
    };

}