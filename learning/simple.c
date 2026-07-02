#include <stdio.h>
#include <stdint.h>

typedef struct {
	uint32_t registros[32];
	uint32_t pc;
	uint8_t memoria[1024];
} CPU;

void ejecutar_add(CPU *cpu, int rd, int rs1, int rs2) {
	if (rd != 0) {
		cpu->registros[rd] = cpu->registros[rs1] + cpu->registros[rs2];
	} else {
		cpu->registros[rd] = 0;
	}
}

int main(void) {
	CPU cpu = {0};
	cpu.registros[1] = 10;
	cpu.registros[2] = 32;
	ejecutar_add(&cpu, 3, 1, 2);
	ejecutar_add(&cpu, 0, 1, 2);
	printf("%u\n%u\n", cpu.registros[3], cpu.registros[0]);

	return 0;
}