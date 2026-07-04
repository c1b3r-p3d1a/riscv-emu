#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MEM_SIZE (64 * 1024)

enum {
    OPCODE_R_TYPE = 0x33,
    OPCODE_I_TYPE = 0x13
};

typedef struct {
    uint32_t registros[32];
    uint32_t pc;
    uint8_t memoria[MEM_SIZE];
} CPU;

void cpu_init(CPU *cpu);
uint32_t cpu_fetch(CPU *cpu);
void cpu_decode_execute(CPU *cpu, uint32_t instruction);
void add(CPU *cpu, int rd, int rs1, int rs2);
void sub(CPU *cpu, int rd, int rs1, int rs2);
void xor_op(CPU *cpu, int rd, int rs1, int rs2);
void or_op(CPU *cpu, int rd, int rs1, int rs2);
void and_op(CPU *cpu, int rd, int rs1, int rs2);
void sll(CPU *cpu, int rd, int rs1, int rs2);
void srl(CPU *cpu, int rd, int rs1, int rs2);
void sra(CPU *cpu, int rd, int rs1, int rs2);
void slt(CPU *cpu, int rd, int rs1, int rs2);
void sltu(CPU *cpu, int rd, int rs1, int rs2);
int32_t sign_extended(uint32_t val, int org_bits);
void addi(CPU *cpu, int rd, int rs1, int imm);
void xori(CPU *cpu, int rd, int rs1, int imm);
void ori(CPU *cpu, int rd, int rs1, int imm);
void andi(CPU *cpu, int rd, int rs1, int imm);

#endif