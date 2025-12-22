CC := clang
CFLAGS := -std=c99 -Wall -Wextra $(shell pkg-config --cflags sdl3) -Iinclude -lcglm
LDFLAGS := $(shell pkg-config --libs sdl3) -lvulkan -lm -lcglm

SRC := $(wildcard src/*.c src/vulkan/*.c tests/*.c)
BIN := build/asimotive3d_test

# shaders
GLSLANG := glslangValidator
VSH_SRC := shaders/triangle.vert
FSH_SRC := shaders/triangle.frag
VSH_SPV := shaders/triangle.vert.spv
FSH_SPV := shaders/triangle.frag.spv


ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG -g
	BUILD_MODE := DEBUG
else
	# CFLAGS += -DNDEBUG -O2
	CFLAGS += -DDEBUG -g
	BUILD_MODE := RELEASE
endif

all: $(BIN)

$(BIN): $(SRC) $(VSH_SPV) $(FSH_SPV)
	BUILD_MODE=$(BUILD_MODE)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS)

$(VSH_SPV): $(VSH_SRC)
	$(GLSLANG) -V $< -o $@

$(FSH_SPV): $(FSH_SRC)
	$(GLSLANG) -V $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build

compile_flags:
	@echo "generating compile_flags.txt"
	@rm -f compile_flags.txt
	@for flag in $(CFLAGS); do echo $$flag >> compile_flags.txt; done

.PHONY: all debug run clean compile_flags
