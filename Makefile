CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -std=c11
SRC = src/main.c src/cpu.c src/elf_loader.c
OUT = riscv-emu

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)

.PHONY: all clean