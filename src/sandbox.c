#include "resource_limit.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>


const size_t max_memory = ((size_t) 50) << 20;


void recurse(int i) {
  if (i % 1000 == 0) {
    fprintf(stderr, "%i\n", i);
  }
  recurse(i + 1);
}


int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

  int pid = fork();
  if (pid == -1) {
    perror("failed to fork");
    exit(1);
  } else if (pid == 0) {
    // Child process.

    // Set up resource limits.
    if (set_resource_limit(RLIMIT_AS, max_memory, "failed to set memory limit")) exit(1);
    if (set_resource_limit(RLIMIT_CPU, 1, "failed to set CPU limit")) exit(1);

    // Simulate misbehaving code.
    recurse(0);
    for (;;);
    exit(0);

  } else {
    // Parent process.

    printf("pid: %i\n", pid);
    int status;
    pid = wait(&status);
    if (pid == -1) {
      perror("failed to wait for child");
      exit(1);
    }
    printf("child %i exited with status code %i\n", pid, status);
  }

  return 0;
}
