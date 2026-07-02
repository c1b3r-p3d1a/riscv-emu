#include <stdio.h>
#include "cpu.h"

int main(void) {
    CPU cpu;
    cpu_init(&cpu);

    cpu.memoria[0] = 0xb3;
    cpu.memoria[1] = 0x87;
    cpu.memoria[2] = 0xe7;
    cpu.memoria[3] = 0x40;

    cpu.registros[15] = 10;
    cpu.registros[14] = 32;
    
    for (int i = 0; i < 1; i++) {
        uint32_t instruction = cpu_fetch(&cpu);
        cpu_decode_execute(&cpu, instruction);
        cpu.pc += 4;
    }

    return 0;
}