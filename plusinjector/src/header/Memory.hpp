#pragma once

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>

namespace pi {

    class Memory {
    public:
        explicit Memory(HANDLE hProcess);
        ~Memory();

        Memory(const Memory&) = delete;
        Memory& operator=(const Memory&) = delete;

        std::string ReadString(uintptr_t addr, size_t maxLen);
        std::string ReadUnicodeString(uintptr_t addr, size_t byteLen);
        int16_t     ReadShort(uintptr_t addr);
        int32_t     ReadInt(uintptr_t addr);
        int64_t     ReadLong(uintptr_t addr);
        std::vector<uint8_t> ReadBytes(uintptr_t addr, size_t size);

        template<typename T>
        T Read(uintptr_t addr) {
            T v{};
            auto b = ReadBytes(addr, sizeof(T));
            std::memcpy(&v, b.data(), sizeof(T));
            return v;
        }

        uintptr_t Allocate(size_t size);
        void      Write(uintptr_t addr, const void* data, size_t size);
        void      Write(uintptr_t addr, const std::vector<uint8_t>& data) {
            Write(addr, data.data(), data.size());
        }

        uintptr_t AllocateAndWrite(const std::vector<uint8_t>& data);
        uintptr_t AllocateAndWrite(const std::string& str);
        uintptr_t AllocateAndWrite(const char* str);
        uintptr_t AllocateAndWrite(int32_t v);
        uintptr_t AllocateAndWrite(int64_t v);

    private:
        HANDLE _handle = nullptr;
        std::unordered_map<uintptr_t, size_t> _allocations;
    };

}