#include <stdio.h>
#include <stdint.h>

typedef struct {
	uint32_t registros[32];
	uint32_t pc;
	uint8_t memoria[1024];
} CPU;

int main(void) {
	CPU cpu;
	int indice = 0;

	if ((indice != 0)) {
		cpu.registros[indice] = 100;
		indice += 1;
	} else {
		cpu.registros[0] = 0;
		indice += 1;
	}

	printf("%u\n%u\n", cpu.registros[0], cpu.registros[5]);

	return 0;
}