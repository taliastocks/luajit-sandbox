#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "mmap_stack.h"

#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>


size_t MAX_STACK_SIZE;

size_t PAGE_SIZE;
size_t STACK_TOP_ESTIMATE;


static void segv_handler(int sig, siginfo_t *si, void *context) {
  (void) sig;
  (void) context;

  void *addr = (void *) ((size_t) si->si_addr & ~(PAGE_SIZE - 1));

  // If we're within the stack, try to grow the stack.
  if ((size_t) addr > STACK_TOP_ESTIMATE - MAX_STACK_SIZE) {
    addr = mmap(addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (addr != MAP_FAILED) {
      return;
    }
    perror("failed to allocate stack space");
  } else if ((size_t) addr >= STACK_TOP_ESTIMATE - MAX_STACK_SIZE - PAGE_SIZE) {
    fputs("stack limit exceeded\n", stderr);
  } else {
    fputs("segmentation fault\n", stderr);
  }

  // Remove signal handler and re-raise signal.
  signal(sig, SIG_DFL);
  raise(sig);
}


int mmap_stack_init_stack(void) {
  int stack_address_dummy;
  (void) stack_address_dummy;

  PAGE_SIZE = sysconf(_SC_PAGESIZE);
  STACK_TOP_ESTIMATE = ((size_t) &stack_address_dummy) & ~(PAGE_SIZE - 1); // Close enough.

  struct rlimit rlim;
  if (getrlimit(RLIMIT_STACK, &rlim)) {
    perror("failed to get current stack limit");
    return 1;
  }
  if (rlim.rlim_cur == RLIM_INFINITY) {
    MAX_STACK_SIZE = ((size_t) 8) << 20;
  } else {
    MAX_STACK_SIZE = rlim.rlim_cur;
  }

  // Create an alternate signal stack.
  stack_t ss;
  ss.ss_size = 2 * SIGSTKSZ;
  ss.ss_sp = mmap(NULL, ss.ss_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ss.ss_sp == MAP_FAILED) {
    perror("failed to allocate alternate signal stack");
    return 1;
  }
  ss.ss_flags = 0;
  if (sigaltstack(&ss, NULL) == -1) {
    perror("failed to set alternate signal stack");
    return 1;
  }

  // Set sigsegv signal handler to grow the stack since it won't grow automatically (RLIMIT_AS).
  struct sigaction sa;
  sa.sa_sigaction = segv_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("failed to set sigsegv signal handler");
    return 1;
  }

  return 0;
}
