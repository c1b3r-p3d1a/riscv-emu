#include <stdio.h>
#include "cpu.h"

void cpu_init(CPU *cpu) {
    for (int i = 0; i < 32; i++) {
        cpu->registros[i] = 0;
    }
    cpu->pc = 0;
    for (int i = 0; i < MEM_SIZE; i++) {
        cpu->memoria[i] = 0;
    }
}

uint32_t cpu_fetch(CPU *cpu) {
    uint32_t b0 = cpu->memoria[cpu->pc + 0];
    uint32_t b1 = cpu->memoria[cpu->pc + 1];
    uint32_t b2 = cpu->memoria[cpu->pc + 2];
    uint32_t b3 = cpu->memoria[cpu->pc + 3];

    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

static uint32_t extract(uint32_t instruction, int pos, int len) {
    return (instruction >> pos) & ((1 << len) - 1);
}

void add(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = cpu->registros[rs1] + cpu->registros[rs2];
    }
}

void sub(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = cpu->registros[rs1] - cpu->registros[rs2];
    }
}

void cpu_decode_execute(CPU *cpu, uint32_t instruction) {
    // printf("Instrucción leída: 0x%08X (aún no decodificada)\n", instruction);
    int opcode = extract(instruction, 0, 7);
    switch (opcode) {
        case OPCODE_R_TYPE: {
            int func3 = extract(instruction, 12, 3);
            int func7 = extract(instruction, 25, 7);
            int rd = extract(instruction, 7, 5);
            int rs1 = extract(instruction, 15, 5);
            int rs2 = extract(instruction, 20, 5);

            if (func3 == 0b000) {
                if (func7 == 0b0000000) {
                    add(cpu, rd, rs1, rs2);
                    printf("0x%08x = %d\n", rd, (int32_t)cpu->registros[rd]);
                } else if (func7 == 0b0100000) {
                    sub(cpu, rd, rs1, rs2);
                    printf("0x%08x = %d\n", rd, (int32_t)cpu->registros[rd]);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}