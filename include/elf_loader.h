#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include "cpu.h"

uint32_t read_u32_le(uint8_t *buffer, int offset);
uint16_t read_u16_le(uint8_t *buffer, int offset);
bool load_elf(CPU *cpu, const char *file_path, uint32_t *entry_point_out);

#endif