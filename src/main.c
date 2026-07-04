#include <stdio.h>
#include "cpu.h"

int main(void) {
    CPU cpu;
    cpu_init(&cpu);

    cpu.memory[0] = 0x13;
    cpu.memory[1] = 0x01;
    cpu.memory[2] = 0x02;
    cpu.memory[3] = 0x02;

    cpu.reg[15] = 10;
    cpu.reg[14] = 32;
    
    for (int i = 0; i < 1; i++) {
        uint32_t instruction = cpu_fetch(&cpu);
        cpu_decode_execute(&cpu, instruction);
        cpu.pc += 4;
    }

    return 0;
}