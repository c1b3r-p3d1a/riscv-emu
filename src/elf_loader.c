#include <stdio.h>
#include <cpu.h>
#include <elf_loader.h>

static uint32_t read_u32_le(uint8_t *buffer, int offset) {
    return buffer[offset] | (buffer[offset+1] << 8) | (buffer[offset+2] << 16) | (buffer[offset+3] << 24);
}

static uint16_t read_u16_le(uint8_t *buffer, int offset) {
    return buffer[offset] | (buffer[offset+1] << 8);
}


bool load_elf(CPU *cpu, const char *file_path, uint32_t *entry_point_out) {
    FILE *f = fopen(file_path, "rb");

    if (f==NULL) {
        return false;
    }

    uint8_t header[52];
    size_t read = fread(header, 1, 52, f);

    if (read != 52) {
        fclose(f);

        return false;
    }

    int MAGIC[4] = {0x7F, 0x45, 0x4C, 0x46};

    for (int i = 0; i < 4; i++){
        if (MAGIC[i] != header[i]) {
            fclose(f);
            
            return false;
        }
    }

    uint32_t entry_point = read_u32_le(header, 24);
    uint32_t start_program_header = read_u32_le(header, 28);
    uint16_t size_program_header = read_u16_le(header, 42);
    uint16_t num_program_header = read_u16_le(header, 44);

    *entry_point_out = entry_point;

    printf("0x%04x %u %u %u\n", entry_point, start_program_header, size_program_header, num_program_header);

    return true;
}