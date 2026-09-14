#pragma once

#include <windows.h>
#include <stdexcept>
#include <string>

namespace pi {

    class PlusInjectorException : public std::runtime_error {
    public:
        explicit PlusInjectorException(const std::string& msg)
            : std::runtime_error(msg) {}

        PlusInjectorException(const std::string& msg, DWORD lastError)
            : std::runtime_error(
                msg + " (Win32 error: " + std::to_string(lastError) + ")") {
        }
    };

}