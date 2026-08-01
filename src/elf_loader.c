#include <stdio.h>
#include "cpu.h"
#include "elf_loader.h"

uint32_t read_u32_le(uint8_t *buffer, int offset) {
    return buffer[offset] | (buffer[offset+1] << 8) | (buffer[offset+2] << 16) | (buffer[offset+3] << 24);
}

uint16_t read_u16_le(uint8_t *buffer, int offset) {
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

    // printf("0x%04x %u %u %u\n", entry_point, start_program_header, size_program_header, num_program_header);

    for (int i = 0; i < num_program_header; i++) {
        uint32_t entry_pos = start_program_header + i * size_program_header;

        fseek(f, entry_pos, SEEK_SET);

        uint8_t ph_buffer[32];

        size_t read = fread(ph_buffer, 1, 32, f);

        if (read != 32) {
            fclose(f);

            return false;
        }

        uint32_t type = read_u32_le(ph_buffer, 0);
        if (type == 1) {
            uint32_t offset = read_u32_le(ph_buffer, 4);
            uint32_t virt_addr = read_u32_le(ph_buffer, 8);
            uint32_t file_siz = read_u32_le(ph_buffer, 16);
            uint32_t mem_siz = read_u32_le(ph_buffer, 20);

            // printf("0x%04x 0x%04x 0x%04x 0x%04x\n", offset, virt_addr, file_siz, mem_siz);

            if ((virt_addr - MEM_BASE + mem_siz) > MEM_SIZE) {
                fclose(f);

                return false;
            }

            fseek(f, offset, SEEK_SET);

            size_t read = fread(&cpu->memory[virt_addr - MEM_BASE], 1, file_siz, f);

            if (read != file_siz) {
                fclose(f);

                return false;
            }

            for (uint32_t j = file_siz; j < mem_siz; j++) {
                cpu->memory[(virt_addr - MEM_BASE) + j] = 0;
            }
        }

    }

    *entry_point_out = entry_point;
    fclose(f);

    return true;
}