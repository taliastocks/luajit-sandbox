/*
  Initialize a sandbox on Linux.
  Copyright (C) 2017 Collin RM Stocks. All rights reserved.
*/

#define _GNU_SOURCE
#include <sched.h>

#include "sandbox.h"

#include <errno.h>
#include <seccomp.h> // libseccomp
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>


volatile bool sandbox_cpu_exceeded = false;


#define err(v, msg) do { if (v) { perror(msg); return 1; } } while (0)


static void catch_xcpu(int sig) {
  /*
    Signal handler to set the sandbox_cpu_exceeded flag and update the current
    CPU limit to the hard limit. Does not report errors, since the program
    state is unknown and IO might be unsafe.
  */
  (void) sig;
  sandbox_cpu_exceeded = true;
  struct rlimit lim;
  if (getrlimit(RLIMIT_CPU, &lim))
    return; // it's not safe to output an error here
  lim.rlim_cur = lim.rlim_max;
  setrlimit(RLIMIT_CPU, &lim); // no point detecting errors here
}


static void catch_sys(int sig) {
  (void) sig;
  exit(1);
}


int sandbox_init(const struct sandbox_settings *sandbox_settings) {
  struct sigaction action;
  // Set up signal handler for detecting reaching the soft CPU limit.
  memset(&action, '\0', sizeof(action));
  action.sa_handler = &catch_xcpu;
  action.sa_flags = SA_RESETHAND; // only call the signal handler once
  err(sigaction(SIGXCPU, &action, NULL), "failed to set soft CPU limit handler");
  // Set up signal handler for cleanly exiting on SIGSYS instead of dumping core.
  memset(&action, '\0', sizeof(action));
  action.sa_handler = &catch_sys;
  err(sigaction(SIGSYS, &action, NULL), "failed to set sandbox violation exit handler");

  // Set up resource limits.
  struct rlimit lim;
  lim.rlim_cur = 0;
  lim.rlim_max = 0;
  err(setrlimit(RLIMIT_CORE, &lim), "failed to set core size limit");
  lim.rlim_cur = sandbox_settings->max_memory;
  if (lim.rlim_cur == 0) lim.rlim_cur = RLIM_INFINITY; // interpret zero as unlimited memory
  lim.rlim_max = lim.rlim_cur;
  err(setrlimit(RLIMIT_AS, &lim), "failed to set memory limit");
  lim.rlim_cur = sandbox_settings->max_cpu_time;
  if (lim.rlim_cur == 0) lim.rlim_cur = RLIM_INFINITY; // interpret zero as unlimited CPU
  lim.rlim_max = (lim.rlim_cur == RLIM_INFINITY) ? RLIM_INFINITY : (lim.rlim_cur + 1);
  err(setrlimit(RLIMIT_CPU, &lim), "failed to set CPU limit");

  // Switch to a new user namespace. This has the effect of dropping any capabilities
  // held in the parent namespace.
  err(unshare(CLONE_NEWUSER), "failed to switch to a new user namespace");

  // Initialize the seccomp filter.
  scmp_filter_ctx ctx;
  ctx = seccomp_init(SCMP_ACT_TRAP); // send SIGSYS on violation (catchable)

  // System call whitelist:
  int any_errors = 0;
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigreturn), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mremap), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(dup), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(dup2), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(dup3), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fcntl), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);

  // Return errors on the following system calls:
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(access), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(lstat), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(open), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(readlink), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(stat), 0);

  // Allow getting and setting RLIMIT_CPU:
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getrlimit), 1, SCMP_A0(SCMP_CMP_EQ, RLIMIT_CPU));
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(setrlimit), 1, SCMP_A0(SCMP_CMP_EQ, RLIMIT_CPU));

  any_errors |= seccomp_load(ctx); // apply the filter

  seccomp_release(ctx); // release the memory storing the filter

  if (any_errors) {
    fputs("failed to load seccomp filter", stderr);
    return 1;
  }

  return 0;
}
