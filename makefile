$(shell mkdir -p bin)
$(shell mkdir -p build)

CC := gcc -W -Wall -Wextra -pedantic -Werror -std=c11

.PHONY: default
default: bin/sandbox

.PHONY: clean
clean:
	rm -rf bin/* build/*

.PHONY: run
run: bin/sandbox
	time -p ./$<

bin/sandbox: src/sandbox.c build/resource_limit.o build/mmap_stack.o
	$(CC) $+ -o $@

build/mmap_stack.o: src/mmap_stack.c src/mmap_stack.h build/resource_limit.o
build/resource_limit.o: src/resource_limit.c src/resource_limit.h

build/%.o:
	$(CC) -c $< -o $@
