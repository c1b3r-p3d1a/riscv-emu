#include <stdio.h>
#include "cpu.h"
#include "elf_loader.h"
#include "kernel_loader.h"

bool load_kernel_image(CPU *cpu, const char *file_path, uint32_t *entry_point_out) {
    FILE *f = fopen(file_path, "rb");

    if (f==NULL) {
        return false;
    }

    uint8_t header[64];
    size_t read = fread(header, 1, 64, f);

    if (read != 64) {
        fclose(f);

        return false;
    }

    uint32_t MAGIC = 0x05435352;

    uint32_t magic_bytes = read_u32_le(header, 56);

    if (MAGIC != magic_bytes) {
        fclose(f);

        return false;
    }

    uint32_t text_offset_low = read_u32_le(header, 8);
    uint32_t text_offset_high = read_u32_le(header, 12);
    uint32_t image_size_low = read_u32_le(header, 16);
    uint32_t image_size_high = read_u32_le(header, 20);

    if ((text_offset_high != 0) || (image_size_high != 0)) {
        fclose(f);

        return false;
    }

    fseek(f, 0, SEEK_SET);

    size_t read_full = fread(&cpu->memory[text_offset_low], 1, image_size_low, f);

    if (read_full != image_size_low) {
        fclose(f);

        return false;
    }

    *entry_point_out = MEM_BASE + text_offset_low;

    return true;
}