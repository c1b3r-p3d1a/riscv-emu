#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include "cpu.h"

bool load_elf(CPU *cpu, const char *file_path, uint32_t *entry_point_out);

#endif