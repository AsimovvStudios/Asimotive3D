CC := clang
CFLAGS := -std=c99 -Wall -Wextra $(shell pkg-config --cflags sdl3) -Iinclude
LDFLAGS := $(shell pkg-config --libs sdl3) -lvulkan

SRC := src/*.c tests/*.c
BIN := build/asimotive3d_test

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

debug: CFLAGS += -DDEBUG -g
debug: clean all

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build

compile_flags:
	@echo "generating compile_flags.txt"
	@rm -f compile_flags.txt
	@for flag in $(CFLAGS); do echo $$flag >> compile_flags.txt; done

.PHONY: all debug run clean compile_flags
