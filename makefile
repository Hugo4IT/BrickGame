CC=clang
CCFLAGS=-Iinc -arch $(ARCH) -Wall -Wextra -ggdb -O0 -MMD -MF bin/$*.d `pkg-config --cflags --static sdl2 sdl2_ttf`
ARCH=x86_64
LINKER=clang
LINKFLAGS=-arch $(ARCH) `pkg-config --libs --static sdl2 sdl2_ttf`
LEAKCHECKER=valgrind

OUTPUT=bin/brickgame

SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,bin/%.o,$(SOURCES))
DEPENDS := $(patsubst src/%.c,bin/%.d,$(SOURCES))

.PHONY: default
default:
	@mkdir -p src bin inc

clean: default
	@rm bin/*
build: default $(OUTPUT)
run: build
	@$(OUTPUT)
leakcheck: build
	@$(LEAKCHECKER) -- $(OUTPUT)

bin/%.o: src/%.c makefile
	@echo 'Compiling: $@ ($<)'
	@$(CC) $(CCFLAGS) -c -o $@ $<

-include $(DEPENDS)

$(OUTPUT): $(OBJECTS)
	@echo 'Linking: $@ ($^)'
	@$(LINKER) $(LINKFLAGS) -o $@ $^