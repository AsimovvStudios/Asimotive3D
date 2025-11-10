CC := clang
CFLAGS := -std=c99 -Wall -Wextra $(shell pkg-config --cflags sdl3) -Iinclude
LDFLAGS := $(shell pkg-config --libs sdl3) -lvulkan -lm

SRC := src/*.c tests/*.c
SRC += src/vulkan/*.c
BIN := build/asimotive3d_test

ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG -g
	BUILD_MODE := DEBUG
else
	CFLAGS += -DNDEBUG -O2
	BUILD_MODE := RELEASE
endif

all: $(BIN)

$(BIN): $(SRC)
	BUILD_MODE=$(BUILD_MODE)
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

.PHONY: all debug run clean compile_flags
