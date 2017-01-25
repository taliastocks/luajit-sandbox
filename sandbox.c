#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>


const size_t max_stack_size = ((size_t) 20) << 20;
const size_t stack_buffer = ((size_t) 5) << 20;
const size_t max_memory = ((size_t) 50) << 20;

size_t page_size;
size_t stack_top_estimate;


static void segv_handler(int sig, siginfo_t *si, void *context) {
  (void) sig;
  (void) context;

  void *addr = (void *) ((size_t) si->si_addr & ~(page_size - 1)); // Mask the address to a page boundary.

  // If we're within the stack, try to grow the stack.
  if ((size_t) addr > stack_top_estimate - max_stack_size) {
    addr = mmap(addr, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (addr != MAP_FAILED) {
      return;
    }
    perror("failed to allocate stack space");
  } else if ((size_t) addr > stack_top_estimate - max_stack_size - stack_buffer) {
    fputs("stack limit exceeded\n", stderr);
  } else {
    fputs("segmentation fault\n", stderr);
  }

  // Remove signal handler and re-raise signal.
  signal(sig, SIG_DFL);
  raise(sig);
}


static inline void set_resource_limit(int resource, rlim_t value, const char *failure_message) {
  struct rlimit lim;
  lim.rlim_cur = lim.rlim_max = value;
  if (setrlimit(resource, &lim) != 0) {
    perror(failure_message);
    exit(1);
  }
}


void recurse(int i) {
  if (i % 1000 == 0) {
    printf("%i\n", i);
  }
  recurse(i + 1);
}


int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

  page_size = sysconf(_SC_PAGESIZE);
  stack_top_estimate = (size_t) &argc;

  int pid = fork();
  if (pid == -1) {
    perror("failed to fork");
    exit(1);
  } else if (pid == 0) {
    // Child process.

    // Create an alternate signal stack.
    stack_t ss;
    ss.ss_size = 2 * SIGSTKSZ;
    ss.ss_sp = mmap(NULL, ss.ss_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ss.ss_sp == MAP_FAILED) {
      perror("failed to allocate alternate signal stack");
      exit(1);
    }
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == -1) {
      perror("failed to set alternate signal stack");
      exit(1);
    }

    // Set sigsegv signal handler to grow the stack since it won't grow automatically (RLIMIT_AS).
    struct sigaction sa;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
      perror("failed to set sigsegv signal handler");
      exit(1);
    }

    // Set up resource limits.
    set_resource_limit(RLIMIT_AS, max_memory, "failed to set memory limit");
    set_resource_limit(RLIMIT_CPU, 1, "failed to set CPU limit");

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
