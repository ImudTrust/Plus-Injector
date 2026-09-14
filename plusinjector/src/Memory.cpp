#include "header/Memory.hpp"
#include "header/Native.hpp"
#include "header/Exceptions.hpp"
#include <Windows.h>
#include <algorithm>

namespace pi {

    Memory::Memory(HANDLE h) : _handle(h) {}

    Memory::~Memory() {
        for (auto& kv : _allocations) {
            native::VirtualFreeEx(_handle, kv.first, 0, MEM_RELEASE);
        }
    }

    std::string Memory::ReadString(uintptr_t addr, size_t maxLen) {
        if (addr == 0) {
            throw PlusInjectorException("Cannot read a string from a null address");
        }

        // Most Mono names are short, so reading them all at once is faster.
        // If the read fails, fall back to reading one byte at a time.
        std::vector<uint8_t> buf(maxLen);
        if (native::ReadProcessMemory(_handle, addr, buf.data(), maxLen)) {
            const auto end = std::find(buf.begin(), buf.end(), uint8_t{ 0 });
            return std::string(buf.begin(), end);
        }

        std::string out;
        out.reserve(maxLen);
        for (size_t i = 0; i < maxLen; ++i) {
            uint8_t b = 0;
            if (!native::ReadProcessMemory(_handle, addr + i, &b, 1)) {
                throw PlusInjectorException(
                    "Failed to read process memory", GetLastError());
            }
            if (b == 0) break;
            out.push_back(static_cast<char>(b));
        }
        return out;
    }

    std::string Memory::ReadUnicodeString(uintptr_t addr, size_t byteLen) {
        if (byteLen == 0) return {};
        std::vector<uint8_t> buf(byteLen);
        if (!native::ReadProcessMemory(_handle, addr, buf.data(), byteLen)) {
            throw PlusInjectorException(
                "Failed to read process memory", GetLastError());
        }
        std::wstring w;
        w.reserve(byteLen / 2);
        for (size_t i = 0; i + 1 < byteLen; i += 2) {
            wchar_t c = static_cast<wchar_t>(buf[i] | (buf[i + 1] << 8));
            w.push_back(c);
        }
        if (w.empty()) return {};
        int needed = WideCharToMultiByte(CP_UTF8, 0, w.c_str(),
            (int)w.size(),
            nullptr, 0, nullptr, nullptr);
        std::string out(needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
            out.data(), needed, nullptr, nullptr);
        return out;
    }

    int16_t Memory::ReadShort(uintptr_t a) { return Read<int16_t>(a); }
    int32_t Memory::ReadInt(uintptr_t a) { return Read<int32_t>(a); }
    int64_t Memory::ReadLong(uintptr_t a) { return Read<int64_t>(a); }

    std::vector<uint8_t> Memory::ReadBytes(uintptr_t addr, size_t size) {
        std::vector<uint8_t> buf(size);
        if (!native::ReadProcessMemory(_handle, addr, buf.data(), size)) {
            throw PlusInjectorException(
                "Failed to read process memory", GetLastError());
        }
        return buf;
    }

    uintptr_t Memory::Allocate(size_t size) {
        uintptr_t addr = native::VirtualAllocEx(
            _handle, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (addr == 0) {
            throw PlusInjectorException(
                "Failed to allocate process memory", GetLastError());
        }
        _allocations[addr] = size;
        return addr;
    }

    void Memory::Write(uintptr_t addr, const void* data, size_t size) {
        if (!native::WriteProcessMemory(_handle, addr, data, size)) {
            throw PlusInjectorException(
                "Failed to write process memory", GetLastError());
        }
    }

    uintptr_t Memory::AllocateAndWrite(const std::vector<uint8_t>& data) {
        uintptr_t a = Allocate(data.size());
        if (!data.empty()) Write(a, data.data(), data.size());
        return a;
    }

    uintptr_t Memory::AllocateAndWrite(const std::string& s) {
        std::vector<uint8_t> v(s.begin(), s.end());
        v.push_back(0);
        return AllocateAndWrite(v);
    }

    uintptr_t Memory::AllocateAndWrite(const char* s) {
        return AllocateAndWrite(std::string(s));
    }

    uintptr_t Memory::AllocateAndWrite(int32_t v) {
        return AllocateAndWrite(std::vector<uint8_t>(
            reinterpret_cast<uint8_t*>(&v),
            reinterpret_cast<uint8_t*>(&v) + 4));
    }

    uintptr_t Memory::AllocateAndWrite(int64_t v) {
        return AllocateAndWrite(std::vector<uint8_t>(
            reinterpret_cast<uint8_t*>(&v),
            reinterpret_cast<uint8_t*>(&v) + 8));
    }

}
