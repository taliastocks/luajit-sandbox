#define _GNU_SOURCE
#include <sched.h>

#include "sandbox.h"
#include "resource_limit.h"

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

  return 0;
}
