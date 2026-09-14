#include "header/Assembler.hpp"
#include <cstring>

namespace pi {

    static void Append64(std::vector<uint8_t>& v, uint64_t x) {
        uint8_t b[8];
        std::memcpy(b, &x, 8);
        v.insert(v.end(), b, b + 8);
    }

    static void Append32(std::vector<uint8_t>& v, uint32_t x) {
        uint8_t b[4];
        std::memcpy(b, &x, 4);
        v.insert(v.end(), b, b + 4);
    }

    void Assembler::MovRax(uintptr_t a) {
        code_.push_back(0x48); code_.push_back(0xB8); Append64(code_, a);
    }
    void Assembler::MovRcx(uintptr_t a) {
        code_.push_back(0x48); code_.push_back(0xB9); Append64(code_, a);
    }
    void Assembler::MovRdx(uintptr_t a) {
        code_.push_back(0x48); code_.push_back(0xBA); Append64(code_, a);
    }
    void Assembler::MovR8(uintptr_t a) {
        code_.push_back(0x49); code_.push_back(0xB8); Append64(code_, a);
    }
    void Assembler::MovR9(uintptr_t a) {
        code_.push_back(0x49); code_.push_back(0xB9); Append64(code_, a);
    }

    void Assembler::SubRsp(uint8_t a) {
        code_.push_back(0x48); code_.push_back(0x83);
        code_.push_back(0xEC); code_.push_back(a);
    }
    void Assembler::AddRsp(uint8_t a) {
        code_.push_back(0x48); code_.push_back(0x83);
        code_.push_back(0xC4); code_.push_back(a);
    }
    void Assembler::CallRax() {
        code_.push_back(0xFF); code_.push_back(0xD0);
    }
    void Assembler::MovRaxTo(uintptr_t dest) {
        code_.push_back(0x48); code_.push_back(0xA3); Append64(code_, dest);
    }

    void Assembler::Push(uintptr_t arg) {
        if ((int64_t)arg < 128) {
            code_.push_back(0x6A);
            code_.push_back((uint8_t)(arg & 0xFF));
        }
        else {
            code_.push_back(0x68);
            Append32(code_, (uint32_t)arg);
        }
    }

    void Assembler::MovEax(uint32_t arg) {
        code_.push_back(0xB8);
        Append32(code_, arg);
    }
    void Assembler::CallEax() {
        code_.push_back(0xFF); code_.push_back(0xD0);
    }
    void Assembler::AddEsp(uint8_t a) {
        code_.push_back(0x83); code_.push_back(0xC4); code_.push_back(a);
    }
    void Assembler::MovEaxTo(uintptr_t dest) {
        code_.push_back(0xA3); Append32(code_, (uint32_t)dest);
    }
    void Assembler::Return() { code_.push_back(0xC3); }

}
