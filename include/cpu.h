#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#define OPCODE_MASK     0b01111111
#define OPCODE_LUI      0b00110111
#define OPCODE_AUIPC    0b00010111
#define OPCODE_JAL      0b01101111
#define OPCODE_JALR     0b01100111
#define OPCODE_BRANCH   0b01100011
#define OPCODE_LOAD     0b00000011
#define OPCODE_STORE    0b00100011
#define OPCODE_ALUI     0b00010011
#define OPCODE_ALU      0b00110011

struct CPU
{
    uint8_t* memory;

    uint32_t x[32];
    uint32_t pc;

    int dirty = -1;

    CPU(uint8_t* _memory) : memory(_memory) {}

    void reset();
    void execute(uint32_t instruction);
    void step();
    std::string disassemble(uint32_t instruction);

private:

    uint8_t func7;
    uint8_t func3;
    uint8_t dest;
    uint8_t src1;
    uint8_t src2;
    uint32_t imm;

    void decode_R_type(uint32_t inst);
    void decode_I_type(uint32_t inst);
    void decode_S_type(uint32_t inst);
    void decode_B_type(uint32_t inst);
    void decode_U_type(uint32_t inst);
    void decode_J_type(uint32_t inst);

    void lui(uint32_t inst);
    void auipc(uint32_t inst);
    void jal(uint32_t inst);
    void jalr(uint32_t inst);
    void branch(uint32_t inst);
    void load(uint32_t inst);
    void store(uint32_t inst);
    void alu(uint32_t inst, bool immediate);

    std::string disassemble_alu(uint32_t inst, bool immediate);
};

extern const char* reg_name[];

std::string fmt(const char* fmt, ...);