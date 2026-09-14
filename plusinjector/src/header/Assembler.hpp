#pragma once

#include <cstdint>
#include <vector>

namespace pi 
{

    class Assembler {
    public:
        void MovRax(uintptr_t arg);
        void MovRcx(uintptr_t arg);
        void MovRdx(uintptr_t arg);
        void MovR8 (uintptr_t arg);
        void MovR9 (uintptr_t arg);

        void SubRsp(uint8_t arg);
        void AddRsp(uint8_t arg);
        void CallRax();
        void MovRaxTo(uintptr_t dest);

        void Push(uintptr_t arg);
        void MovEax(uint32_t arg);
        void CallEax();
        void AddEsp(uint8_t arg);
        void MovEaxTo(uintptr_t dest);

        void Return();

        const std::vector<uint8_t>& Bytes() const { return code_; }
        std::vector<uint8_t> ToByteArray() const { return code_; }

    private:
        std::vector<uint8_t> code_;
    };

}
