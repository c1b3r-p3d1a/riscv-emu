#include <stdio.h>
#include <stdbool.h>
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

void write_memory(CPU *cpu, uint32_t addr, uint32_t value, int num_bytes) {
    for (int i = 0; i < num_bytes; i++) {
        cpu->memory[addr + i] = (value >> i*8) & 0xFF;
    }
}

bool evaluate_condition(int func3, uint32_t val_rs1, uint32_t val_rs2) {
    switch (func3) {
        case 0b000: {
            return val_rs1 == val_rs2;
        }
        case 0b001: {
            return (val_rs1 != val_rs2) ? true : false;
        }
        case 0b100: {
            return ((int32_t)val_rs1 < (int32_t)val_rs2) ? true : false;
        }
        case 0b101: {
            return ((int32_t)val_rs1 >= (int32_t)val_rs2) ? true : false;
        }
        case 0b110: {
            return (val_rs1 < val_rs2) ? true : false;
        }
        case 0b111: {
            return (val_rs1 >= val_rs2) ? true : false; 
        }
        default: {
            return false;
        }
    }
}

static void add(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] + cpu->reg[rs2];
    }
}

static void sub(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] - cpu->reg[rs2];
    }
}

static void xor_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] ^ cpu->reg[rs2];
    }
}

static void or_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] | cpu->reg[rs2];
    }
}

static void and_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] & cpu->reg[rs2];
    }
}

static void sll(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] << (cpu->reg[rs2] & 0x1F);
    }
}

static void srl(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] >> (cpu->reg[rs2] & 0x1F);
    }
}

static void sra(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = (int32_t)(cpu->reg[rs1]) >> (cpu->reg[rs2] & 0x1F);
    }
}

static void slt(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = ((int32_t)(cpu->reg[rs1]) < (int32_t)(cpu->reg[rs2]) ? 1 : 0);
    }
}

static void sltu(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = (cpu->reg[rs1] < cpu->reg[rs2] ? 1 : 0);
    }
}

static void addi(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] + (uint32_t)imm;
    }
}

static void xori(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] ^ (uint32_t)imm;
    }
}

static void ori(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] | (uint32_t)imm;
    }
}

static void andi(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] & (uint32_t)imm;
    }
}

static void slli(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] << (imm & 0x1F);
    }
}

static void srli(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] >> (imm & 0x1F);
    }
}

static void srai(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = (int32_t)(cpu->reg[rs1]) >> (imm & 0x1F);
    }
}

static void slti(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = ((int32_t)(cpu->reg[rs1]) < (int32_t)(imm) ? 1 : 0);
    }
}

static void sltiu(CPU *cpu, int rd, int rs1, int imm) {
    if (rd != 0) {
        cpu->reg[rd] = (cpu->reg[rs1] < (uint32_t)imm) ? 1 : 0;
    }
}

static void lb(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        uint32_t read = read_memory(cpu, addr, 1);
        cpu->reg[rd] = sign_extended(read, 8);
    }
}

static void lh(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        uint32_t read = read_memory(cpu, addr, 2);
        cpu->reg[rd] = sign_extended(read, 16);
    }
}

static void lw(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        cpu->reg[rd] = read_memory(cpu, addr, 4);
    }
}

static void lbu(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        cpu->reg[rd] = read_memory(cpu, addr, 1);
    }
}

static void lhu(CPU *cpu, int rd, int addr) {
    if (rd != 0) {
        cpu->reg[rd] = read_memory(cpu, addr, 2);
    }
}

static void sb(CPU *cpu, int rs2, int addr) {
    write_memory(cpu, addr, cpu->reg[rs2], 1);
}

static void sh(CPU *cpu, int rs2, int addr) {
    write_memory(cpu, addr, cpu->reg[rs2], 2);
}

static void sw(CPU *cpu, int rs2, int addr) {
    write_memory(cpu, addr, cpu->reg[rs2], 4);
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
            cpu->pc += 4;
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
            cpu->pc += 4;
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
            cpu->pc += 4;
            break;
        }
        case OPCODE_S_TYPE: {
            int func3 = extract(instruction, 12, 3);
            int imm_high = extract(instruction, 25, 7);
            int imm_low = extract(instruction, 7, 5);
            int imm_complete = (imm_high << 5) | imm_low;
            int imm = sign_extended(imm_complete, 12);
            int rs1 = extract(instruction, 15, 5);
            int rs2 = extract(instruction, 20, 5);
            int addr = cpu->reg[rs1] + imm;

            if (func3 == 0b000) {
                sb(cpu, rs2, addr);
            } else if (func3 == 0b001) {
                sh(cpu, rs2, addr);
            } else if (func3 == 0b010) {
                sw(cpu, rs2, addr);
            }
            cpu->pc += 4;
            break;
        }
        case OPCODE_B_TYPE: {
            int func3 = extract(instruction, 12, 3);
            int imm12 = extract(instruction, 31, 1);
            int imm10_5 = extract(instruction, 25, 6);
            int imm4_1 = extract(instruction, 8, 4);
            int imm11 = extract(instruction, 7, 1);
            int imm_complete = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1);
            int imm = sign_extended(imm_complete, 13);
            int rs1 = extract(instruction, 15, 5);
            int rs2 = extract(instruction, 20, 5);

            if (evaluate_condition(func3, cpu->reg[rs1], cpu->reg[rs2])) {
                cpu->pc += imm;
            } else {
                cpu->pc += 4;
            }
            break;
        }
        case OPCODE_U_LOAD_TYPE: {
            int imm = extract(instruction, 12, 20) << 12;
            int rd = extract(instruction, 7, 5);

            if (rd != 0) {
                cpu->reg[rd] = imm;
            }
            
            cpu->pc += 4;
            break;
        }
        case OPCODE_U_ADD_TYPE: {
            int imm = extract(instruction, 12, 20) << 12;
            int rd = extract(instruction, 7, 5);
            
            if (rd != 0) {
                cpu->reg[rd] = cpu->pc + imm;
            }
            cpu->pc += 4;
            break;
        }
        case OPCODE_J_TYPE: {
            int imm20 = extract(instruction, 31, 1);
            int imm10_1 = extract(instruction, 21, 10);
            int imm11 = extract(instruction, 20, 1);
            int imm19_12 = extract(instruction, 12, 8);
            int imm_complete = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
            int imm = sign_extended(imm_complete, 21);
            int rd = extract(instruction, 7, 5);

            uint32_t dest = cpu->pc + imm;
            
            if (rd != 0) {
                cpu->reg[rd] = cpu->pc + 4;
            }

            cpu->pc = dest;
            break;
        }
        case OPCODE_I_JUMP_TYPE: {
            int imm = sign_extended(extract(instruction, 20, 12), 12);
            int rd = extract(instruction, 7, 5);
            int rs1 = extract(instruction, 15, 5);

            uint32_t dest = (cpu->reg[rs1] + imm) & ~1;
            
            if (rd != 0) {
                cpu->reg[rd] = cpu->pc + 4;
            }

            cpu->pc = dest;
            break;
        }
        default: {
            break;
        }
    }
}