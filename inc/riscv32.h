#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>

typedef enum FMT { R, I, S, B, U, J } FMT;

typedef struct Instruction 
{
    /* instruction shorthand */
    char inst[16];

    /* instruction format type */
    FMT type;

    /* static parts of instruction format */
    uint32_t base;
} Instruction;

/* instruction builder */
#define R_TYPE(INST, OP, FUNCT3, FUNCT7)    (Instruction){.inst = INST, .type = R, .base = (OP & 0b1111111) + ((FUNCT3 & 0b111) << 12) + ((FUNCT7 & 0b1111111) << 25)}
#define I_TYPE(INST, OP, FUNCT3)            (Instruction){.inst = INST, .type = I, .base = (OP & 0b1111111) + ((FUNCT3 & 0b111) << 12)}
#define SI_TYPE(INST, OP, FUNCT3, FUNCT7)   (Instruction){.inst = INST, .type = I, .base = (OP & 0b1111111) + ((FUNCT3 & 0b111) << 12) + ((FUNCT7 & 0b1111111) << 25)}
#define S_TYPE(INST, OP, FUNCT3)            (Instruction){.inst = INST, .type = S, .base = (OP & 0b1111111) + ((FUNCT3 & 0b111) << 12)}
#define B_TYPE(INST, OP, FUNCT3)            (Instruction){.inst = INST, .type = B, .base = (OP & 0b1111111) + ((FUNCT3 & 0b111) << 12)}
#define U_TYPE(INST, OP)                    (Instruction){.inst = INST, .type = U, .base = (OP & 0b1111111)}
#define J_TYPE(INST, OP)                    (Instruction){.inst = INST, .type = J, .base = (OP & 0b1111111)}

typedef struct Register 
{
    /* ABI name */
    char name[16];

    /* index */
    uint8_t number;
} Register;

#define REGISTER(ABI, I)                    (Register){.name = ABI, .number = I}

/*
    Array containing everysingle instruction and needed data.
*/
static const Instruction instructions[] = 
{
    /* RV32I instructions 
       source: https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
    */

    /* Register type instructions */

    /*      INST     OPCODE    FUNCT3  FUNCT7 */

    R_TYPE("add",   0b0110011,  0x0,    0x00),
    R_TYPE("sub",   0b0110011,  0x0,    0x20),
    R_TYPE("xor",   0b0110011,  0x4,    0x00),
    R_TYPE("or",    0b0110011,  0x6,    0x00),
    R_TYPE("and",   0b0110011,  0x7,    0x00),
    R_TYPE("sll",   0b0110011,  0x1,    0x00),
    R_TYPE("srl",   0b0110011,  0x5,    0x00),
    R_TYPE("sra",   0b0110011,  0x5,    0x20),
    R_TYPE("slt",   0b0110011,  0x2,    0x00),
    R_TYPE("sltu",  0b0110011,  0x3,    0x00),

    /* Immediate type instructions */

    I_TYPE("addi",  0b0010011,  0x0),
    I_TYPE("xori",  0b0010011,  0x4),
    I_TYPE("ori",   0b0010011,  0x6),
    I_TYPE("andi",  0b0010011,  0x7),
    I_TYPE("slti",  0b0010011,  0x2),
    I_TYPE("sltiu", 0b0010011,  0x3),
    I_TYPE("lb",    0b0000011,  0x0),
    I_TYPE("lh",    0b0000011,  0x1),
    I_TYPE("lw",    0b0000011,  0x2),
    I_TYPE("lbu",   0b0000011,  0x4),
    I_TYPE("lhu",   0b0000011,  0x5),
    I_TYPE("jalr",  0b1100111,  0x0),
    I_TYPE("ecall", 0b1110011,  0x0),

    /* special immediate type instructions with funct7 */

    SI_TYPE("slli", 0b0010011,  0x1,    0x00),
    SI_TYPE("srli", 0b0010011,  0x5,    0x00),
    SI_TYPE("srai", 0b0010011,  0x5,    0x20),

    /* Store type instructions */

    S_TYPE("sb",    0b0100011,  0x0),
    S_TYPE("sh",    0b0100011,  0x1),
    S_TYPE("sw",    0b0100011,  0x2),

    /* Branch type instructions */

    B_TYPE("beq",   0b1100011,  0x0),
    B_TYPE("bne",   0b1100011,  0x1),
    B_TYPE("blt",   0b1100011,  0x4),
    B_TYPE("bge",   0b1100011,  0x5),
    B_TYPE("bltu",  0b1100011,  0x6),
    B_TYPE("bgeu",  0b1100011,  0x7),

    /* Upper immediate type instructions */

    U_TYPE("lui",   0b0110111),
    U_TYPE("auipc", 0b0010111),

    /* Jump type instructions */

    J_TYPE("jal",   0b1101111),
};

/*
    Array containing all register aliasses
*/
static const Register registers[] = 
{
    /* all registers x0-31 */
    REGISTER("x0",  0),
    REGISTER("x1",  1),
    REGISTER("x2",  2),
    REGISTER("x3",  3),
    REGISTER("x4",  4),
    REGISTER("x5",  5),
    REGISTER("x6",  6),
    REGISTER("x7",  7),
    REGISTER("x8",  8),
    REGISTER("x9",  9),
    REGISTER("x10", 10),
    REGISTER("x11", 11),
    REGISTER("x12", 12),
    REGISTER("x13", 13),
    REGISTER("x14", 14),
    REGISTER("x15", 15),
    REGISTER("x16", 16),
    REGISTER("x17", 17),
    REGISTER("x18", 18),
    REGISTER("x19", 19),
    REGISTER("x20", 20),
    REGISTER("x21", 21),
    REGISTER("x22", 22),
    REGISTER("x23", 23),
    REGISTER("x24", 24),
    REGISTER("x25", 25),
    REGISTER("x26", 26),
    REGISTER("x27", 27),
    REGISTER("x28", 28),
    REGISTER("x29", 29),
    REGISTER("x30", 30),
    REGISTER("x31", 31),

    /* ABI names */
    REGISTER("zero",0),
    REGISTER("ra",  1),
    REGISTER("sp",  2),
    REGISTER("gp",  3),
    REGISTER("tp",  4),
    REGISTER("t0",  5),
    REGISTER("x1",  6),
    REGISTER("t2",  7),
    REGISTER("fp",  8),
    REGISTER("s0",  8),
    REGISTER("s1",  9),
    REGISTER("a0",  10),
    REGISTER("a1",  11),
    REGISTER("a2",  12),
    REGISTER("a3",  13),
    REGISTER("a4",  14),
    REGISTER("a5",  15),
    REGISTER("a6",  16),
    REGISTER("a7",  17),
    REGISTER("s2",  18),
    REGISTER("s3",  19),
    REGISTER("s4",  20),
    REGISTER("s5",  21),
    REGISTER("s6",  22),
    REGISTER("s7",  23),
    REGISTER("s8",  24),
    REGISTER("s9",  25),
    REGISTER("s10", 26),
    REGISTER("s11", 27),
    REGISTER("t3",  28),
    REGISTER("t4",  29),
    REGISTER("t5",  30),
    REGISTER("t6",  31),
};

#endif