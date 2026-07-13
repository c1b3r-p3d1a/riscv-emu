#include <stdio.h>
#include "cpu.h"

int main(void) {
    CPU cpu;
    cpu_init(&cpu);
    cpu.reg[1] = 0x244;   // simula que 'ra' ya tiene guardada la dirección de retorno

    cpu.memory[0] = 0x67;
    cpu.memory[1] = 0x80;
    cpu.memory[2] = 0x00;
    cpu.memory[3] = 0x00;
    cpu.pc = 0;   // <- corregido, igual que en JAL

    uint32_t instr2 = cpu_fetch(&cpu);
    cpu_decode_execute(&cpu, instr2);

    printf("Tras JALR -> pc = 0x%X (esperado: 0x244)\n", cpu.pc);

    return 0;
}