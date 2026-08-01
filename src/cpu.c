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
    cpu->mode = 3;
}

uint32_t cpu_fetch(CPU *cpu, bool *fail) {
    bool error;
    uint32_t physc_pc = translate_mmu(cpu, cpu->pc, ACCESS_EXEC, &error);

    if (error) {
        trap(cpu, 12);
        *fail = true;
        return 0;
    }

    *fail = false;

    uint32_t b0 = cpu->memory[physc_pc + 0];
    uint32_t b1 = cpu->memory[physc_pc + 1];
    uint32_t b2 = cpu->memory[physc_pc + 2];
    uint32_t b3 = cpu->memory[physc_pc + 3];

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

static uint32_t get_field(uint32_t val, int pos, int len) {
    return (val >> pos) & ((1 << len) - 1);
}

static uint32_t set_field(uint32_t val, int pos, int len, uint32_t new) {
    return  ((val) & ~(((1 << len) - 1) << pos)) | (new << pos);
}

void trap(CPU *cpu, uint32_t cause) {
    bool delegate = (cpu->mode != 3) && (get_field(cpu->csr[MEDELEG], cause, 1) == 1);

    if (delegate) {
        cpu->csr[SSTATUS] = set_field(cpu->csr[SSTATUS], 8, 1, cpu->mode);
        cpu->csr[SSTATUS] = set_field(cpu->csr[SSTATUS], 5, 1, get_field(cpu->csr[SSTATUS], 1, 1));
        cpu->csr[SSTATUS] = set_field(cpu->csr[SSTATUS], 1, 1, 0);
        
        cpu->mode = 1;
        
        cpu->csr[SEPC] = cpu->pc;
        cpu->csr[SCAUSE] = cause;
        cpu->pc = cpu->csr[STVEC];
    } else {
        cpu->csr[MSTATUS] = set_field(cpu->csr[MSTATUS], 11, 2, cpu->mode);
        cpu->csr[MSTATUS] = set_field(cpu->csr[MSTATUS], 7, 1, get_field(cpu->csr[MSTATUS], 3, 1));
        cpu->csr[MSTATUS] = set_field(cpu->csr[MSTATUS], 3, 1, 0);

        cpu->mode = 3;

        cpu->csr[MEPC] = cpu->pc;
        cpu->csr[MCAUSE] = cause;
        cpu->pc = cpu->csr[MTVEC];
    }
}

uint32_t translate_mmu(CPU *cpu, uint32_t virt_addr, int access_t, bool *error) {
    if (get_field(cpu->csr[SATP], 31, 1) == 0) {
        *error = false;
        return virt_addr;
    } else {
        uint32_t vpn1 = get_field(virt_addr, 22, 10);
        uint32_t vpn0 = get_field(virt_addr, 12, 10);
        uint32_t offset = get_field(virt_addr, 0, 12);

        uint32_t level1_table = get_field(cpu->csr[SATP], 0, 22) * 4096;
        uint32_t pte1 = read_memory(cpu, level1_table + vpn1*4, 4);

        if (get_field(pte1, 0, 1) == 0) {
            *error = true;
            return 0;
        }

        uint32_t level2_table = get_field(pte1, 10, 22) * 4096;
        uint32_t pte2 = read_memory(cpu, level2_table + vpn0*4, 4);

        if ((get_field(pte2, 2, 1) == 1) && (get_field(pte2, 1, 1) == 0)) {
            *error = true;
            return 0;
        } else if (get_field(pte2, access_t, 1) == 1) {
            if (((get_field(pte2, 4, 1) == 0) && (cpu->mode == 0)) | ((get_field(pte2, 4, 1) == 1) && ((cpu->mode == 1) | (cpu->mode == 3)))) {
                *error = true;
                return 0;
            }
        } else {
            *error = true;
            return 0;
        }

        *error = false;
        return (get_field(pte2, 10, 22) * 4096) + offset;
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

static void mul(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        cpu->reg[rd] = cpu->reg[rs1] * cpu->reg[rs2];
    }
}

static void mulh(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        int64_t val1 = (int64_t)(int32_t)cpu->reg[rs1];
        int64_t val2 = (int64_t)(int32_t)cpu->reg[rs2];
        int64_t result = val1 * val2;

        cpu->reg[rd] = (int32_t)(result >> 32);
    }
}

static void mulhsu(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        int64_t val1 = (int64_t)(int32_t)cpu->reg[rs1];
        uint64_t val2 = (uint64_t)(uint32_t)cpu->reg[rs2];
        int64_t result = val1 * (int64_t)val2;

        cpu->reg[rd] = (uint32_t)((uint64_t)result >> 32);
    }
}

static void mulhu(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        uint64_t val1 = (uint64_t)(uint32_t)cpu->reg[rs1];
        uint64_t val2 = (uint64_t)(uint32_t)cpu->reg[rs2];
        uint64_t result = val1 * val2;

        cpu->reg[rd] = (uint32_t)(result >> 32);
    }
}

static void div_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        int32_t dividend = (int32_t)cpu->reg[rs1];
        int32_t divider = (int32_t)cpu->reg[rs2];

        if (divider == 0) {
            cpu->reg[rd] = (uint32_t)-1;
        } else if (dividend == INT32_MIN && divider == -1) {
            cpu->reg[rd] = (uint32_t)INT32_MIN;
        } else {
            cpu->reg[rd] = (uint32_t)(dividend / divider);
        }
    }
}

static void rem_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        int32_t dividend = (int32_t)cpu->reg[rs1];
        int32_t divider = (int32_t)cpu->reg[rs2];

        if (divider == 0) {
            cpu->reg[rd] = (uint32_t)dividend;
        } else if (dividend == INT32_MIN && divider == -1) {
            cpu->reg[rd] = 0;
        } else {
            cpu->reg[rd] = (uint32_t)(dividend % divider);
        }
    }
}

static void divu_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        uint32_t dividend = cpu->reg[rs1];
        uint32_t divider = cpu->reg[rs2];

        if (divider == 0) {
            cpu->reg[rd] = (uint32_t)-1;
        } else {
            cpu->reg[rd] = (uint32_t)(dividend / divider);
        }
    }
}

static void remu_op(CPU *cpu, int rd, int rs1, int rs2) {
    if (rd != 0) {
        uint32_t dividend = cpu->reg[rs1];
        uint32_t divider = cpu->reg[rs2];

        if (divider == 0) {
            cpu->reg[rd] = (uint32_t)dividend;
        } else {
            cpu->reg[rd] = (uint32_t)(dividend % divider);
        }
    }
}

static void csrrw(CPU *cpu, int rd, uint32_t valor, int csr) {
    if (rd != 0) {
        uint32_t value = cpu->csr[csr];
        cpu->reg[rd] = value;
    }

    cpu->csr[csr] = valor;
}

static void csrrs(CPU *cpu, int rd, uint32_t valor, int csr) {
    uint32_t value = cpu->csr[csr];
    
    if (rd != 0) {
        cpu->reg[rd] = value;
    }

    if (valor != 0) {
        cpu->csr[csr] = value | valor;
    }
}

static void csrrc(CPU *cpu, int rd, uint32_t valor, int csr) {
    uint32_t value = cpu->csr[csr];
    
    if (rd != 0) {
        cpu->reg[rd] = value;
    }

    if (valor != 0) {
        cpu->csr[csr] = value & ~(valor);
    }
}

static void amoadd(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    write_memory(cpu, addr, read + cpu->reg[rs2], 4);
}

static void amoswap(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    write_memory(cpu, addr, cpu->reg[rs2], 4);
}

static void amoxor(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    write_memory(cpu, addr, read ^ cpu->reg[rs2], 4);
}

static void amoand(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    write_memory(cpu, addr, read & cpu->reg[rs2], 4);
}

static void amoor(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    write_memory(cpu, addr, read | cpu->reg[rs2], 4);
}

static void amomin(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    uint32_t new = ((int32_t)read < (int32_t)cpu->reg[rs2]) ? read : cpu->reg[rs2];
    write_memory(cpu, addr, new, 4);
}

static void amomax(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    uint32_t nuevo = ((int32_t)read > (int32_t)cpu->reg[rs2]) ? read : cpu->reg[rs2];
    write_memory(cpu, addr, nuevo, 4);
}

static void amominu(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    uint32_t new = (read < cpu->reg[rs2]) ? read : cpu->reg[rs2];
    write_memory(cpu, addr, new, 4);
}

static void amomaxu(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    uint32_t nuevo = (read > cpu->reg[rs2]) ? read : cpu->reg[rs2];
    write_memory(cpu, addr, nuevo, 4);
}

static void lr(CPU *cpu, int rd, int rs2, uint32_t addr) {
    uint32_t read = read_memory(cpu, addr, 4);

    if (rd != 0) {
        cpu->reg[rd] = read;
    }

    cpu->reserve_addr = addr;
    cpu->reserve_active = true;
}

static void sc(CPU *cpu, int rd, int rs2, uint32_t addr) {
    if (cpu->reserve_active && (cpu->reserve_addr == addr)) {
        write_memory(cpu, addr, cpu->reg[rs2], 4);

        if (rd != 0) {
            cpu->reg[rd] = 0;
        }
    } else {
        if (rd != 0) {
            cpu->reg[rd] = 1;
        }
    }
    
    cpu->reserve_active = false;
}

void cpu_decode_execute(CPU *cpu, uint32_t instruction, bool *terminated) {
    int opcode = extract(instruction, 0, 7);
    *terminated = false;

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
                } else if (func7 == 0b0100000) {
                    sub(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0000001) {
                    mul(cpu, rd, rs1, rs2);
                }
            } else if (func3 == 0b100) {
                if (func7 == 0b0000000) {
                    xor_op(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0000001) {
                    div_op(cpu, rd, rs1, rs2);
                }
            } else if (func3 == 0b110) {
                if (func7 == 0b0000000) {
                    or_op(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0000001) {
                    rem_op(cpu, rd, rs1, rs2);
                }
            } else if (func3 == 0b111) {
                if (func7 == 0b0000000) {
                    and_op(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0000001) {
                    remu_op(cpu, rd, rs1, rs2);
                }
            } else if (func3 == 0b001) {
                if (func7 == 0b0000000) {
                    sll(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0000001) {
                    mulh(cpu, rd, rs1, rs2);
                }
            } else if (func3 == 0b101) {
                if (func7 == 0b0000000) {
                    srl(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0100000) {
                    sra(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0000001) {
                    divu_op(cpu, rd, rs1, rs2);
                }
            } else if (func3 == 0b010) {
                if (func7 == 0b0000000) {
                    slt(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0000001) {
                    mulhsu(cpu, rd, rs1, rs2);
                }
            } else if (func3 == 0b011) {
                if (func7 == 0b0000000) {
                    sltu(cpu, rd, rs1, rs2);
                } else if (func7 == 0b0000001) {
                    mulhu(cpu, rd, rs1, rs2);
                }
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
                    if (func7 == 0b0000000) {
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
            uint32_t virt_addr = cpu->reg[rs1] + imm;

            if ((func3 == 0b001) && (virt_addr % 2 != 0)) {
                trap(cpu, 4);
            } else if ((func3 == 0b010) && (virt_addr % 4 != 0)) {
                trap(cpu, 4);
            } else if ((func3 == 0b101) && (virt_addr % 2 != 0)) {
                trap(cpu, 4);
            } else {
                bool error;
                uint32_t addr = translate_mmu(cpu, virt_addr, ACCESS_READ, &error);
    
                if (error) {
                    trap(cpu, 13);
                } else {
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
                }
            }
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
            uint32_t virt_addr = cpu->reg[rs1] + imm;

            if ((func3 == 0b001) && (virt_addr % 2 != 0)) {
                trap(cpu, 6);
            } else if ((func3 == 0b010) && (virt_addr % 4 != 0)) {
                trap(cpu, 6);
            } else {
                bool error;
                uint32_t addr = translate_mmu(cpu, virt_addr, ACCESS_WRITE, &error);
    
                if (error) {
                    trap(cpu, 15);
                } else {
                    if (addr == 0x10000000) {
                        putchar((char)cpu->reg[rs2]);
                    } else if ((addr == 0x100000) && (cpu->reg[rs2] == 0x5555)) {
                        *terminated = true;
                    } else {
                        if (func3 == 0b000) {
                            sb(cpu, rs2, addr);
                        } else if (func3 == 0b001) {
                            sh(cpu, rs2, addr);
                        } else if (func3 == 0b010) {
                            sw(cpu, rs2, addr);
                        }
                    }
                    cpu->pc += 4;
                }
            }  


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
        case OPCODE_SYSTEM: {
            int func3 = extract(instruction, 12, 3);
            int imm = sign_extended(extract(instruction, 20, 12), 12);
            int rd = extract(instruction, 7, 5);
            int rs1 = extract(instruction, 15, 5);
            int csr = extract(instruction, 20, 12);

            if (func3 == 0b000) {
                if (imm == 0b000) {
                    trap(cpu, 8 + cpu->mode);
                } else if (imm == 0b001) {
                    // ebreak
                } else if (imm == 0b1100000010) {
                    cpu->pc = cpu->csr[MEPC];
                    
                    cpu->mode = get_field(cpu->csr[MSTATUS], 11, 2);
                    cpu->csr[MSTATUS] = set_field(cpu->csr[MSTATUS], 3, 1, get_field(cpu->csr[MSTATUS], 7, 1));
                    cpu->csr[MSTATUS] = set_field(cpu->csr[MSTATUS], 7, 1, 1);

                    cpu->csr[MSTATUS] = set_field(cpu->csr[MSTATUS], 11, 2, 0);
                } else if (imm == 0b000100000010) {
                    cpu->pc = cpu->csr[SEPC];

                    cpu->mode = get_field(cpu->csr[SSTATUS], 8, 1);
                    cpu->csr[SSTATUS] = set_field(cpu->csr[SSTATUS], 1, 1, get_field(cpu->csr[SSTATUS], 5, 1));
                    cpu->csr[SSTATUS] = set_field(cpu->csr[SSTATUS], 5, 1, 1);
                    cpu->csr[SSTATUS] = set_field(cpu->csr[SSTATUS], 8, 1, 0);
                }
            } else if (func3 == 0b001) {
                csrrw(cpu, rd, cpu->reg[rs1], csr); 
                cpu->pc += 4;
            } else if (func3 == 0b010) {
                csrrs(cpu, rd, cpu->reg[rs1], csr);
                cpu->pc += 4;
            } else if (func3 == 0b011) {
                csrrc(cpu, rd, cpu->reg[rs1], csr);
                cpu->pc += 4;
            } else if (func3 == 0b101) {
                csrrw(cpu, rd, (uint32_t)rs1, csr);
                cpu->pc += 4;
            } else if (func3 == 0b110) {
                csrrs(cpu, rd, (uint32_t)rs1, csr);
                cpu->pc += 4;
            } else if (func3 == 0b111) {
                csrrc(cpu, rd, (uint32_t)rs1, csr);
                cpu->pc += 4;
            }

            break;
        }
        case OPCODE_MISC_MEM: {
            cpu->pc += 4;
            break;
        }
        case OPCODE_AMO: {
            int func5 = extract(instruction, 27, 5);
            int aq = extract(instruction, 26, 1);
            int rl = extract(instruction, 25, 1);
            int rs2 = extract(instruction, 20, 5);
            int rs1 = extract(instruction, 15, 5);
            int func3 = extract(instruction, 12, 3);
            int rd = extract(instruction, 7, 5);
            uint32_t virt_addr = cpu->reg[rs1];

            bool error_r, error_w;
            uint32_t addr_r = translate_mmu(cpu, virt_addr, ACCESS_READ, &error_r);
            uint32_t addr_w = translate_mmu(cpu, virt_addr, ACCESS_WRITE, &error_w);

            if (error_r || error_w) {
                trap(cpu, 15);
            } else {
                if (func5 == 0b00000) {
                    amoadd(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b00001) {
                    amoswap(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b00010) {
                    lr(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b00011) {
                    sc(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b00100) {
                    amoxor(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b10000) {
                    amomin(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b10100) {
                    amomax(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b01000) {
                    amoor(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b01100) {
                    amoand(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b11000) {
                    amominu(cpu, rd, rs2, addr_r);
                } else if (func5 == 0b11100) {
                    amomaxu(cpu, rd, rs2, addr_r);
                }
                cpu->pc += 4;
            }

            break;
        }
        default: {
            trap(cpu, 2);
            break;
        }
    }
}