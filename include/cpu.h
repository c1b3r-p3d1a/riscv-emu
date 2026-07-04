#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MEM_SIZE (64 * 1024)

enum {
    OPCODE_R_TYPE = 0x33,
    OPCODE_I_ARITHMETIC_TYPE = 0x13,
    OPCODE_I_LOAD_TYPE = 0x03,
    OPCODE_S_TYPE = 0x23
};

typedef struct {
    uint32_t reg[32];
    uint32_t pc;
    uint8_t memory[MEM_SIZE];
} CPU;

void cpu_init(CPU *cpu);
uint32_t cpu_fetch(CPU *cpu);
void cpu_decode_execute(CPU *cpu, uint32_t instruction);
uint32_t read_memory(CPU *cpu, uint32_t addr, int num_bytes);
void write_memory(CPU *cpu, uint32_t addr, uint32_t value, int num_bytes);
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
void slli(CPU *cpu, int rd, int rs1, int imm);
void srli(CPU *cpu, int rd, int rs1, int imm);
void srai(CPU *cpu, int rd, int rs1, int imm);
void slti(CPU *cpu, int rd, int rs1, int imm);
void sltiu(CPU *cpu, int rd, int rs1, int imm);
void lb(CPU *cpu, int rd, int addr);
void lh(CPU *cpu, int rd, int addr);
void lw(CPU *cpu, int rd, int addr);
void lbu(CPU *cpu, int rd, int addr);
void lhu(CPU *cpu, int rd, int addr);
void sb(CPU *cpu, int rs2, int addr);
void sh(CPU *cpu, int rs2, int addr);
void sw(CPU *cpu, int rs2, int addr);

#endif