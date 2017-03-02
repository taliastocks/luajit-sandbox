#define _GNU_SOURCE
#include <sched.h>

#include "sandbox.h"
#include "resource_limit.h"

#include <errno.h>
#include <seccomp.h> // libseccomp
#include <stdio.h>


#define err(v, msg) do { if (v) { perror(msg); return 1; } } while (0)


int sandbox_init(const struct sandbox_settings *sandbox_settings) {
  // Set up resource limits.
  err(set_resource_limit(RLIMIT_CORE, 0), "failed to set core size limit");
  err(set_resource_limit(RLIMIT_AS, sandbox_settings->max_memory), "failed to set memory limit");
  err(set_resource_limit(RLIMIT_CPU, sandbox_settings->max_cpu_time), "failed to set CPU limit");

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
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(dup), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(dup2), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(dup3), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fcntl), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);

  // Return errors on the following system calls:
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(open), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(access), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(stat), 0);
  any_errors |= seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(lstat), 0);

  any_errors |= seccomp_load(ctx); // apply the filter

  seccomp_release(ctx); // release the memory storing the filter

  if (any_errors) {
    fputs("failed to load seccomp filter", stderr);
    return 1;
  }

  return 0;
}
