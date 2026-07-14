#include <stdio.h>
#include "cpu.h"
#include "elf_loader.h"

int main(void) {
    CPU cpu;
    cpu_init(&cpu);

    uint32_t entry_point;
    bool loaded = load_elf(&cpu, "elf_test.elf", &entry_point);

    if (!loaded) {
        printf("Error loading ELF\n");
        return 1;
    }

    printf("Load OK\n");
    return 0;
}