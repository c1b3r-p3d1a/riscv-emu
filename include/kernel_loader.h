#ifndef KERNEL_LOADER_H
#define KERNEL_LOADER_H

#include "cpu.h"

bool load_kernel_image(CPU *cpu, const char *ruta_archivo, uint32_t *entry_point_out);

#endif