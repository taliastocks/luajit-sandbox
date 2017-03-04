$(shell mkdir -p bin)
$(shell mkdir -p build)
$(shell mkdir -p build/usr/local/include)

INCLUDE_FLAGS := -I$(realpath build/usr/local/include)
LDFLAGS := -s -static -lm -lseccomp
CC := gcc -O2 -W -Wall -Wextra -pedantic -Werror -std=c11 $(INCLUDE_FLAGS)
AMALG := amalg

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

bin/exe: build/main.o build/sandbox.o build/luajit_wrapper.o build/usr/local/lib/libluajit-5.1.a build/fake_dl.o
	$(CC) $+ -o $@ $(LDFLAGS)

build/main.o: src/main.c build/usr/local/include/luajit-2.0/lua.h
build/sandbox.o: src/sandbox.c src/sandbox.h
build/luajit_wrapper.o: src/luajit_wrapper.c src/luajit_wrapper.h
build/fake_dl.o: src/fake_dl.c

build/%.o:
	$(CC) -c $< -o $@

build/usr/local/include/luajit-2.0/% build/usr/local/lib/%: third_party/luajit-2.0/Makefile
	cd third_party/luajit-2.0/ && $(MAKE) $(AMALG) && $(MAKE) install DESTDIR=$(realpath build)

third_party/luajit-2.0/Makefile:
	git submodule update third_party/luajit-2.0/
