#include <cpu.h>
#include <stdarg.h>

uint32_t bit_cut(uint32_t value, int a, int b, bool sign = false)
{
    uint32_t mask = (-1 << b) ^ ((-1 << a) << 1);

    if (sign)
        return (int)(value & mask) >> b;

    return (value & mask) >> b;
}

void CPU::reset()
{
    for (int i = 0; i < 32; i++)
        this->x[i] = 0;

    dirty = -1;
}

void CPU::execute(uint32_t inst)
{
    uint8_t opcode = inst & OPCODE_MASK;

    x[0] = 0;

    switch (opcode)
    {
    case OPCODE_LUI:    lui(inst);          break;
    case OPCODE_AUIPC:  auipc(inst);        break;
    case OPCODE_JAL:    jal(inst);          break;
    case OPCODE_JALR:   jalr(inst);         break;
    case OPCODE_BRANCH: branch(inst);       break;
    case OPCODE_LOAD:   load(inst);         break;
    case OPCODE_STORE:  store(inst);        break;
    case OPCODE_ALUI:   alu(inst, true);    break;
    case OPCODE_ALU:    alu(inst, false);   break;
    }
}

void CPU::step()
{
    dirty = -1;

    uint32_t instruction = *((uint32_t*)(memory + pc));
    execute(instruction);
}

void CPU::decode_R_type(uint32_t inst)
{
    func7 = bit_cut(inst, 31, 25);
    func3 = bit_cut(inst, 14, 12);
    dest = bit_cut(inst, 11, 7);
    src1 = bit_cut(inst, 19, 15);
    src2 = bit_cut(inst, 24, 20);
    imm = 0;
}

void CPU::decode_I_type(uint32_t inst)
{
    func7 = 0;
    func3 = bit_cut(inst, 14, 12);
    dest = bit_cut(inst, 11, 7);
    src1 = bit_cut(inst, 19, 15);
    src2 = 0;
    imm = bit_cut(inst, 31, 20, true);
}

void CPU::decode_S_type(uint32_t inst)
{
    func7 = 0;
    func3 = bit_cut(inst, 14, 12);
    dest = 0;
    src1 = bit_cut(inst, 19, 15);
    src2 = bit_cut(inst, 24, 20);
    imm = (bit_cut(inst, 31, 25, true) << 5) + bit_cut(inst, 11, 7);
}

void CPU::decode_B_type(uint32_t inst)
{
    func7 = 0;
    func3 = bit_cut(inst, 14, 12);
    dest = 0;
    src1 = bit_cut(inst, 19, 15);
    src2 = bit_cut(inst, 24, 20);

    uint32_t bit_12 = bit_cut(inst, 31, 31, true);
    uint32_t bit_10_5 = bit_cut(inst, 30, 25);
    uint32_t bit_4_1 = bit_cut(inst, 11, 8);
    uint32_t bit_11 = bit_cut(inst, 7, 7);

    imm = (bit_12 << 12) + (bit_10_5 << 5) + (bit_4_1 << 1) + (bit_11 << 11);
}

void CPU::decode_U_type(uint32_t inst)
{
    func7 = 0;
    func3 = 0;
    dest = bit_cut(inst, 11, 7);
    src1 = 0;
    src2 = 0;
    imm = bit_cut(inst, 31, 12) << 12;
}

void CPU::decode_J_type(uint32_t inst)
{
    func7 = 0;
    func3 = 0;
    dest = bit_cut(inst, 11, 7);
    src1 = 0;
    src2 = 0;

    uint32_t bit_20 = bit_cut(inst, 31, 31, true);
    uint32_t bit_10_1 = bit_cut(inst, 30, 21);
    uint32_t bit_19_12 = bit_cut(inst, 19, 12);
    uint32_t bit_11 = bit_cut(inst, 20, 20);

    imm = (bit_20 << 20) + (bit_10_1 << 1) + (bit_19_12 << 12) + (bit_11 << 11);
}

void CPU::lui(uint32_t inst)
{
    decode_U_type(inst);

    x[dest] = imm;
    dirty = dest;

    pc += 4;
}

void CPU::auipc(uint32_t inst)
{
    decode_U_type(inst);

    x[dest] = pc + imm;
    dirty = dest;

    pc += 4;
}

void CPU::jal(uint32_t inst)
{
    decode_J_type(inst);

    x[dest] = pc + 4;
    dirty = dest;

    pc += imm;
}

void CPU::jalr(uint32_t inst)
{
    decode_I_type(inst);

    if (func3 != 0)
        throw std::runtime_error("Expected func3 to be zero in jalr instruction");

    uint32_t reg = x[src1];

    x[dest] = pc + 4;
    dirty = dest;

    pc = reg + imm;
}

void CPU::branch(uint32_t inst)
{
    decode_B_type(inst);

    bool result;

    switch (func3)
    {
    case 0b000:     result = (x[src1] == x[src2]);                  break;
    case 0b001:     result = (x[src1] != x[src2]);                  break;
    case 0b100:     result = ((int)x[src1] < (int)x[src2]);         break;
    case 0b101:     result = ((int)x[src1] >= (int)x[src2]);        break;
    case 0b110:     result = (x[src1] < x[src2]);                   break;
    case 0b111:     result = (x[src1] >= x[src2]);                  break;

    default: throw std::runtime_error("Unexpected bit pattern in branch instruction");
    }

    pc += result ? imm : 4;
}

void CPU::load(uint32_t inst)
{
    decode_I_type(inst);

    int address = x[src1] + imm;

    switch (func3)
    {
    case 0b000:     x[dest] = *((int8_t*)&memory[address]);         break;
    case 0b001:     x[dest] = *((int16_t*)&memory[address]);        break;
    case 0b010:     x[dest] = *((uint32_t*)&memory[address]);       break;
    case 0b100:     x[dest] = *((uint8_t*)&memory[address]);        break;
    case 0b101:     x[dest] = *((uint16_t*)&memory[address]);       break;

    default: throw std::runtime_error("Unexpected func3 in load instruction");
    }

    dirty = dest;

    pc += 4;
}

void CPU::store(uint32_t inst)
{
    decode_S_type(inst);

    int address = x[src1] + imm;

    switch (func3)
    {
    case 0b000:     *((uint8_t*)&memory[address]) = (uint8_t)x[src2];       break;
    case 0b001:     *((uint16_t*)&memory[address]) = (uint16_t)x[src2];     break;
    case 0b010:     *((uint32_t*)&memory[address]) = x[src2];               break;

    default: throw std::runtime_error("Unexpected func3 in store instruction");
    }

    pc += 4;
}

void CPU::alu(uint32_t inst, bool immediate)
{
    if (immediate)
        decode_I_type(inst);
    else
        decode_R_type(inst);

    uint32_t value = immediate ? imm : x[src2];

    if (func7 == 0)
    {
        switch (func3)
        {
        case 0b000:     x[dest] = x[src1] + value;                                          break;
        case 0b001:     x[dest] = x[src1] << value;                                         break;
        case 0b010:     x[dest] = ((int)x[src1] < (int)value) ? 1 : 0;                      break;
        case 0b011:     x[dest] = ((unsigned int)x[src1] < (unsigned int)value) ? 1 : 0;    break;
        case 0b100:     x[dest] = x[src1] ^ value;                                          break;
        case 0b101:     x[dest] = (unsigned int)x[src1] >> value;                           break;
        case 0b110:     x[dest] = x[src1] | value;                                          break;
        case 0b111:     x[dest] = x[src1] & value;                                          break;

        default: throw std::runtime_error("Unexpected func3 in alu instruction");
        }
    }
    else if (func7 == 0b0100000)
    {
        switch (func3)
        {
        case 0b000:     x[dest] = x[src1] - value;          break;
        case 0b101:     x[dest] = (int)x[src1] >> value;    break;

        default: throw std::runtime_error("Unexpected func3 in alu instruction");
        }
    }
    else
        throw std::runtime_error("Unexpected func7 in alu instruction");

    dirty = dest;

    pc += 4;
}

const char* reg_name[] =
{
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0","t1","t2",
    "s0","s1",
    "a0","a1","a2","a3","a4","a5","a6","a7",
    "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
    "t3","t4","t5","t6"
};

std::string fmt(const char* fmt, ...)
{
    va_list args;
    va_list args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    int size = vsnprintf(nullptr, 0, fmt, args_copy);
    char buffer[size + 1];
    vsnprintf(buffer, size + 1, fmt, args);

    va_end(args_copy);
    va_end(args);

    return std::string(buffer);
}

std::string CPU::disassemble(uint32_t inst)
{
    uint8_t opcode = inst & OPCODE_MASK;

    switch (opcode)
    {
    case OPCODE_LUI:    decode_U_type(inst);    return fmt("lui %s, %d", reg_name[dest], imm >> 12);
    case OPCODE_AUIPC:  decode_U_type(inst);    return fmt("auipc %s, %d", reg_name[dest], imm >> 12);
    case OPCODE_JAL:    decode_J_type(inst);    return fmt("jal %s, %d", reg_name[dest], imm);
    case OPCODE_JALR:
    {
        decode_I_type(inst);

        if (func3 != 0)
            break;

        return fmt("jalr %s, %d(%s)", reg_name[dest], imm, reg_name[src1]);
    }
    case OPCODE_BRANCH:
    {
        decode_B_type(inst);

        std::string res = "";

        switch (func3)
        {
        case 0b000:     res = "beq";        break;
        case 0b001:     res = "bne";        break;
        case 0b100:     res = "blt";        break;
        case 0b101:     res = "bge";        break;
        case 0b110:     res = "bltu";       break;
        case 0b111:     res = "bgeu";       break;

        default: break;
        }

        return fmt("%s %s, %s, %d", res.c_str(), reg_name[src1], reg_name[src2], imm);
    }
    case OPCODE_LOAD:
    {
        decode_I_type(inst);

        int address = x[src1] + imm;

        std::string res = "";

        switch (func3)
        {
        case 0b000:     res = "lb";         break;
        case 0b001:     res = "lh";         break;
        case 0b010:     res = "lw";         break;
        case 0b100:     res = "lbu";        break;
        case 0b101:     res = "lhu";        break;

        default: break;
        }

        return fmt("%s %s, %d(%s)", res.c_str(), reg_name[dest], imm, reg_name[src1]);
    }
    case OPCODE_STORE:
    {
        decode_S_type(inst);

        int address = x[src1] + imm;

        std::string res = "";

        switch (func3)
        {
        case 0b000:     res = "sb";         break;
        case 0b001:     res = "sh";         break;
        case 0b010:     res = "sw";         break;

        default: break;
        }

        return fmt("%s %s, %d(%s)", res.c_str(), reg_name[dest], imm, reg_name[src1]);
    }
    case OPCODE_ALUI:   return disassemble_alu(inst, true);
    case OPCODE_ALU:    return disassemble_alu(inst, false);
    }

    return "Bad instruction";
}

std::string CPU::disassemble_alu(uint32_t inst, bool immediate)
{
    if (immediate)
        decode_I_type(inst);
    else
        decode_R_type(inst);

    std::string res = "";

    if (func7 == 0)
    {
        switch (func3)
        {
        case 0b000:     res = "add";        break;
        case 0b001:     res = "sll";        break;
        case 0b010:     res = "slt";        break;
        case 0b011:     res = "sltu";       break;
        case 0b100:     res = "xor";        break;
        case 0b101:     res = "srl";        break;
        case 0b110:     res = "or";         break;
        case 0b111:     res = "and";        break;

        default: return "Bad instruction";
        }
    }
    else if (func7 == 0b0100000)
    {
        switch (func3)
        {
        case 0b000:     res = "sub";        break;
        case 0b101:     res = "sra";        break;

        default: return "Bad instruction";
        }
    }

    if (immediate)
        return fmt("%si %s, %s, %d", res.c_str(), reg_name[dest], reg_name[src1], imm);

    return fmt("%s %s, %s, %s", res.c_str(), reg_name[dest], reg_name[src1], reg_name[src2]);
}