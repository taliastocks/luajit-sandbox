#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>


int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

  int pid = fork();
  if (pid == -1) {
    perror("failed to fork");
    exit(1);
  } else if (pid == 0) {
    // Child process.
    // Set up rlimits.
    struct rlimit limit;
    limit.rlim_cur = limit.rlim_max = 1;
    if (setrlimit(RLIMIT_CPU, &limit) != 0) {
      perror("failed to set CPU limit");
      exit(1);
    }

    // Simulate misbehaving code.
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
