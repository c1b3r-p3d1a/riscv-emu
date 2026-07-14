#include <stdio.h>
#include "cpu.h"
#include "elf_loader.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Use: ./riscv-emu <your-file.elf>\n");
        return 1;
    }
    
    CPU cpu;
    cpu_init(&cpu);

    uint32_t entry_point;
    bool loaded = load_elf(&cpu, argv[1], &entry_point);

    if (!loaded) {
        printf("Error loading ELF\n");
        return 1;
    }

    printf("Load OK\n");
    cpu.reg[2] = MEM_SIZE - 4;
    cpu.pc = entry_point;

    int MAX_CYCLES = 10000;

    for (int cycle = 0; cycle < MAX_CYCLES; cycle++) {
        uint32_t pc_before = cpu.pc;

        uint32_t instruction = cpu_fetch(&cpu);
        cpu_decode_execute(&cpu, instruction);

        printf("ciclo=%d pc=0x%X sp=0x%X\n", cycle, cpu.pc, cpu.reg[2]);
        
        if (cpu.pc == pc_before) {
            printf("Program finished (program counter unchanged). Exiting...\n");
            printf("Value [0x11118] = %d\n", *(int32_t*)&cpu.memory[0x11118]);
            
            return 0;
        }
    }

    printf("Cycle limit reached, aborting...\n");

    return 1;
}