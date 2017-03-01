$(shell mkdir -p bin)
$(shell mkdir -p build)
$(shell mkdir -p build/include)

INCLUDE_FLAGS := -I$(realpath build/include)
LDFLAGS := -lm -ldl -lseccomp
CC := gcc -O2 -W -Wall -Wextra -pedantic -Werror -std=c11 $(INCLUDE_FLAGS)

.PHONY: default
default: bin/exe

.PHONY: clean
clean:
	rm -rf bin/* build/*

.PHONY: cleaner
cleaner: clean third_party/luajit-2.0/Makefile
	cd third_party/luajit-2.0 && make clean

.PHONY: cleanest
cleanest: clean
	find third_party/luajit-2.0/ -mindepth 1 -maxdepth 1 -exec rm -r {} \+

.PHONY: run
run: bin/exe
	cat "test.lua" | time -p $<; echo "Return status: $$?"

.PHONY: strace
strace: bin/exe
	cat "test.lua" | strace $<; echo "Return status: $$?"

bin/exe: build/main.o build/resource_limit.o build/sandbox.o build/luajit_wrapper.o build/lib/libluajit-5.1.a
	$(CC) $+ -o $@ $(LDFLAGS)

build/main.o: src/main.c build/include/luajit-2.0/lua.h
build/resource_limit.o: src/resource_limit.c src/resource_limit.h
build/sandbox.o: src/sandbox.c src/sandbox.h src/resource_limit.h
build/luajit_wrapper.o: src/luajit_wrapper.c src/luajit_wrapper.h

build/%.o:
	$(CC) -c $< -o $@

build/include/luajit-2.0/% build/lib/%: third_party/luajit-2.0/Makefile
	cd third_party/luajit-2.0/ && $(MAKE) install PREFIX=$(realpath build)

third_party/luajit-2.0/Makefile:
	git submodule update third_party/luajit-2.0/
