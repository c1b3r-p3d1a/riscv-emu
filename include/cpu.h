#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_SIZE (3 * 1024 * 1024)

enum {
    OPCODE_R_TYPE = 0x33,
    OPCODE_I_ARITHMETIC_TYPE = 0x13,
    OPCODE_I_LOAD_TYPE = 0x03,
    OPCODE_S_TYPE = 0x23,
    OPCODE_B_TYPE = 0x63,
    OPCODE_U_LOAD_TYPE = 0x37,
    OPCODE_U_ADD_TYPE = 0x17,
    OPCODE_J_TYPE = 0x6F,
    OPCODE_I_JUMP_TYPE = 0x67,
    OPCODE_SYSTEM = 0x73,
    
    MEPC = 0x341,
    MCAUSE = 0x342,
    MTVEC = 0x305,
    MSTATUS = 0x300
};

typedef struct {
    uint32_t reg[32];
    uint32_t pc;
    uint8_t memory[MEM_SIZE];
    uint32_t csr[4096];
    int mode;
} CPU;

void cpu_init(CPU *cpu);
uint32_t cpu_fetch(CPU *cpu);
void cpu_decode_execute(CPU *cpu, uint32_t instruction, bool *terminated);
uint32_t read_memory(CPU *cpu, uint32_t addr, int num_bytes);
void write_memory(CPU *cpu, uint32_t addr, uint32_t value, int num_bytes);
bool evaluate_condition(int func3, uint32_t val_rs1, uint32_t val_rs2);
int32_t sign_extended(uint32_t val, int org_bits);

#endif