#include <stdio.h>
#include "cpu.h"

void cpu_init(CPU *cpu) {
    for (int i = 0; i < 32; i++) {
        cpu->reg[i] = 0;
    }
    cpu->pc = 0;
    for (int i = 0; i < MEM_SIZE; i++) {
        cpu->memory[i] = 0;
    }
}

uint32_t cpu_fetch(CPU *cpu) {
    uint32_t b0 = cpu->memory[cpu->pc + 0];
    uint32_t b1 = cpu->memory[cpu->pc + 1];
    uint32_t b2 = cpu->memory[cpu->pc + 2];
    uint32_t b3 = cpu->memory[cpu->pc + 3];

    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

static uint32_t extract(uint32_t instruction, int pos, int len) {
    return (instruction >> pos) & ((1 << len) - 1);
}

int32_t sign_extended(uint32_t val, int org_bits) {
    uint32_t mask = (1U << org_bits) - 1;
    uint32_t bit_sign = 1U << (org_bits - 1);

    val = val & mask;

    return (int32_t)((val ^ bit_sign) - bit_sign);
}

uint32_t read_memory(CPU *cpu, uint32_t addr, int num_bytes) {
    uint32_t bytes = 0;
    uint32_t byte_temp = 0;

    for (int i = 0; i < num_bytes; i++) {
        byte_temp = cpu->memory[addr + i];
        bytes = bytes | (byte_temp << i*8);
    }

    return bytes;
}

void add(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] + cpu->reg[rs2];
    }
}

void sub(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] - cpu->reg[rs2];
    }
}

void xor_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] ^ cpu->reg[rs2];
    }
}

void or_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] | cpu->reg[rs2];
    }
}

void and_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] & cpu->reg[rs2];
    }
}

void sll(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] << (cpu->reg[rs2] & 0x1F);
    }
}

void srl(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] >> (cpu->reg[rs2] & 0x1F);
    }
}

void sra(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = (int32_t)(cpu->reg[rs1]) >> (cpu->reg[rs2] & 0x1F);
    }
}

void slt(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = ((int32_t)(cpu->reg[rs1]) < (int32_t)(cpu->reg[rs2]) ? 1 : 0);
    }
}

void sltu(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = (cpu->reg[rs1] < cpu->reg[rs2] ? 1 : 0);
    }
}

void addi(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] + (uint32_t)imm;
    }
}

void xori(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] ^ (uint32_t)imm;
    }
}

void ori(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] | (uint32_t)imm;
    }
}

void andi(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] & (uint32_t)imm;
    }
}

void slli(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] << (imm & 0x1F);
    }
}

void srli(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] >> (imm & 0x1F);
    }
}

void srai(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = (int32_t)(cpu->reg[rs1]) >> (imm & 0x1F);
    }
}

void slti(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = ((int32_t)(cpu->reg[rs1]) < (int32_t)(imm) ? 1 : 0);
    }
}

void sltiu(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = (cpu->reg[rs1] < (uint32_t)imm) ? 1 : 0;
    }
}

void lb(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        uint32_t read = read_memory(cpu, addr, 1);
        cpu->reg[rd] = sign_extended(read, 8);
    }
}

void lh(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        uint32_t read = read_memory(cpu, addr, 2);
        cpu->reg[rd] = sign_extended(read, 16);
    }
}

void lw(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        cpu->reg[rd] = read_memory(cpu, addr, 4);
    }
}

void lbu(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        cpu->reg[rd] = read_memory(cpu, addr, 1);
    }
}

void lhu(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        cpu->reg[rd] = read_memory(cpu, addr, 2);
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
                } else if (func7 == 0b100000) {
                    sub(cpu, rd, rs1, rs2);
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
        case OPCODE_I_ARITHMETIC_TYPE: {
            int func3 = extract(instruction, 12, 3);
            int func7 = extract(instruction, 25, 7);
            int imm = sign_extended(extract(instruction, 20, 12), 12);
            int rd = extract(instruction, 7, 5);
            int rs1 = extract(instruction, 15, 5);
            int shamt = extract(instruction, 20, 5);

            if (func3 == 0b000) {
                addi(cpu, rd, rs1, imm);
            } else if (func3 == 0b100) {
                xori(cpu, rd, rs1, imm);
            } else if (func3 == 0b110) {
                ori(cpu, rd, rs1, imm);
            } else if (func3 == 0b111) {
                andi(cpu, rd, rs1, imm);
                } else if (func3 == 0b001) {
                    slli(cpu, rd, rs1, shamt);
                } else if (func3 == 0b101) {
                    if (func7 == 0b000000) {
                        srli(cpu, rd, rs1, shamt);
                    } else if (func7 == 0b0100000) {
                        srai(cpu, rd, rs1, shamt);
                    }
                } else if (func3 == 0b010) {
                slti(cpu, rd, rs1, imm);
            } else if (func3 == 0b011) {
                sltiu(cpu, rd, rs1, imm);
            }
            break;
        }
        case OPCODE_I_LOAD_TYPE: {
            int func3 = extract(instruction, 12, 3);
            int imm = sign_extended(extract(instruction, 20, 12), 12);
            int rd = extract(instruction, 7, 5);
            int rs1 = extract(instruction, 15, 5);
            uint32_t addr = cpu->reg[rs1] + imm;

            if (func3 == 0b000) {
                lb(cpu, rd, addr);
            } else if (func3 == 0b001) {
                lh(cpu, rd, addr);
            } else if (func3 == 0b010) {
                lw(cpu, rd, addr);
            } else if (func3 == 0b100) {
                lbu(cpu, rd, addr);
            } else if (func3 == 0b101) {
                lhu(cpu, rd, addr);
            }
            break;
        }
        default: {
            break;
        }
    }
}