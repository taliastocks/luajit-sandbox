#include "resource_limit.h"
#include "sandbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>


const size_t max_memory = ((size_t) 50) << 20;


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
    struct sandbox_settings sandbox_settings;
    sandbox_settings.max_memory = max_memory;
    sandbox_settings.max_cpu_time = 1;
    sandbox_init(&sandbox_settings);

    // Simulate misbehaving code.
    for (;;);
    /*
    for (int i; malloc(1 << 20); ++i) {
      printf("%i\n", i);
    }
    */
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
