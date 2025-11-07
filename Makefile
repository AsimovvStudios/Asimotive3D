CC = clang
CFLAGS = -std=c99 -Wall -Wextra $(shell pkg-config --cflags sdl3) -Iinclude
LDFLAGS = $(shell pkg-config --libs sdl3) -lvulkan

SRC = src/main.c
BIN = build/asimotive3d

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build

compile_flags:
	@echo "generating compile_flags.txt"
	@rm -f compile_flags.txt
	@for flag in $(CFLAGS); do echo $$flag >> compile_flags.txt; done

.PHONY: all run clean compile_flags
