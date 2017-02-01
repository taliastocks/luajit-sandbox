#include "sandbox.h"
#include "resource_limit.h"


int sandbox_init(const struct sandbox_settings *sandbox_settings) {
  // Set up resource limits.
  if (set_resource_limit(RLIMIT_CORE, 0, "failed to set core size limit")) return 1; // disables core dump
  if (set_resource_limit(RLIMIT_AS, sandbox_settings->max_memory, "failed to set memory limit")) return 1;
  if (set_resource_limit(RLIMIT_CPU, sandbox_settings->max_cpu_time, "failed to set CPU limit")) return 1;

  return 0;
}
