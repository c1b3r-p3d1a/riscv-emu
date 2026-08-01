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

    int MAX_CYCLES = 100000;
    bool terminated = false;

    for (int cycle = 0; cycle < MAX_CYCLES; cycle++) {
        cpu.mtime += 1;

        uint32_t mtip = get_field(cpu.csr[MIP], 7, 1);
        uint32_t mtie = get_field(cpu.csr[MIE], 7, 1);
        uint32_t mie_global = get_field(cpu.csr[MSTATUS], 3, 1);

        if (cpu.mtime >= cpu.mtimecmp) {
            cpu.csr[MIP] = set_field(cpu.csr[MIP], 7, 1, 1);
            mtip = 1;
        }

        if (mtip && mtie && mie_global) {
            trap(&cpu, 7, true);
            continue;
        }
        
        bool fetch_fail;
        
        uint32_t instruction = cpu_fetch(&cpu, &fetch_fail);

        if (fetch_fail) {
            continue;
        }
        
        cpu_decode_execute(&cpu, instruction, &terminated);
        
        if (terminated) {
            printf("Program finished (syscall). Exiting...\n");
            
            return 0;
        }
    }

    printf("Cycle limit reached, aborting...\n");

    return 1;
}