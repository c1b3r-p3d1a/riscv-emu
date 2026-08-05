#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_BASE 0x80000000
#define MEM_SIZE (256 * 1024 * 1024)

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
    OPCODE_MISC_MEM = 0x0F,
    OPCODE_AMO = 0x2F,
    
    MEPC = 0x341,
    MCAUSE = 0x342,
    MTVEC = 0x305,
    MSTATUS = 0x300,
    
    SEPC = 0x141,
    SCAUSE = 0x142,
    STVEC = 0x105,
    SSTATUS = 0x100,
    MEDELEG = 0x302,
    MIDELEG = 0x303,

    SATP = 0x180,

    ACCESS_READ  = 1,
    ACCESS_WRITE = 2,
    ACCESS_EXEC  = 3,

    MIP=0x344,
    MIE=0x304
};

typedef struct {
    uint32_t reg[32];
    uint32_t pc;
    uint8_t memory[MEM_SIZE];
    uint32_t csr[4096];
    int mode;
    uint32_t reserve_addr;
    bool reserve_active;
    uint64_t mtime;
    uint32_t mtimecmp;
} CPU;

void cpu_init(CPU *cpu);
uint32_t cpu_fetch(CPU *cpu, bool *fail, int *size);
void cpu_decode_execute(CPU *cpu, uint32_t instruction, int instr_size, bool *terminated);
uint32_t read_memory(CPU *cpu, uint32_t addr, int num_bytes);
void write_memory(CPU *cpu, uint32_t addr, uint32_t value, int num_bytes);
bool evaluate_condition(int func3, uint32_t val_rs1, uint32_t val_rs2);
int32_t sign_extended(uint32_t val, int org_bits);
uint32_t get_field(uint32_t val, int pos, int len);
uint32_t set_field(uint32_t val, int pos, int len, uint32_t new);
void trap(CPU *cpu, uint32_t cause, bool is_interruption);
uint32_t translate_mmu(CPU *cpu, uint32_t virt_addr, int access_t, bool *error);
uint32_t compressed_reg(int bits);

#endif