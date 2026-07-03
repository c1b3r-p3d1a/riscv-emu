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

void xor_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = cpu->registros[rs1] ^ cpu->registros[rs2];
    }
}

void or_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = cpu->registros[rs1] | cpu->registros[rs2];
    }
}

void and_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = cpu->registros[rs1] & cpu->registros[rs2];
    }
}

void sll(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = cpu->registros[rs1] << (cpu->registros[rs2] & 0x1F);
    }
}

void srl(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = cpu->registros[rs1] >> (cpu->registros[rs2] & 0x1F);
    }
}

void sra(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = (int32_t)(cpu->registros[rs1]) >> (cpu->registros[rs2] & 0x1F);
    }
}

void slt(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = ((int32_t)(cpu->registros[rs1]) < (int32_t)(cpu->registros[rs2]) ? 1 : 0);
    }
}

void sltu(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->registros[rd] = (cpu->registros[rs1] < cpu->registros[rs2] ? 1 : 0);
    }
}

void cpu_decode_execute(CPU *cpu, uint32_t instruction) {
    int opcode = extract(instruction, 0, 7);
    switch (opcode) {
        case OPCODE_R_TYPE: {
            int func3 = extract(instruction, 12, 3);
            int func7 = extract(instruction, 25, 7);
            int rd = extract(instruction, 7, 5);
            int rs1 = extract(instruction, 15, 5);
            int rs2 = extract(instruction, 20, 5);

            if (func3 == 0b000) {
                if (func7 == 0b000000) {
                    add(cpu, rd, rs1, rs2);
                    printf("0x%08x = %d\n", rd, (int32_t)cpu->registros[rd]);
                } else if (func7 == 0b100000) {
                    sub(cpu, rd, rs1, rs2);
                    printf("0x%08x = %d\n", rd, (int32_t)cpu->registros[rd]);
                }
            } else if (func3 == 0b100) {
                xor_op(cpu, rd, rs1, rs2);
            } else if (func3 == 0b110) {
                or_op(cpu, rd, rs1, rs2);
            } else if (func3 == 0b111) {
                and_op(cpu, rd, rs1, rs2);
            } else if (func3 == 0b001) {
                sll(cpu, rd, rs1, rs2);
            } else if (func3 == 0b101) {
                if (func7 == 0b000000) {
                    srl(cpu, rd, rs1, rs2);
                } else if (func7 == 0b100000) {
                    sra(cpu, rd, rs1, rs2);
                }
            } else if (func3 == 0b010) {
                slt(cpu, rd, rs1, rs2);
            } else if (func3 == 0b011) {
                sltu(cpu, rd, rs1, rs2);
            }
            break;
        }
        default: {
            break;
        }
    }
}