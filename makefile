$(shell mkdir -p bin)

.PHONY: default
default: bin/sandbox

.PHONY: run
run: bin/sandbox
	time -p ./$<

bin/sandbox: sandbox.c
	gcc -W -Wall -Wextra -pedantic -Werror -std=c11 $< -o $@
