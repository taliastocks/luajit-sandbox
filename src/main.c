/*
  Entrypoint.
  Copyright (C) 2017 Collin RM Stocks. All rights reserved.
*/

#include "luajit_wrapper.h"
#include "sandbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


extern char **environ;


const size_t max_memory = ((size_t) 50) << 20;


int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

  // If this process was run with environment variables, clear them all
  // by loading the process image again.
  if (environ[0] != NULL) {
    char *new_environ[] = {NULL};
    execve("/proc/self/exe", argv, new_environ);
    // If execve() returned, it failed.
    perror("failed to restart process without environment");
    return 1;
  }

  // Set up sandbox.
  struct sandbox_settings sandbox_settings;
  sandbox_settings.max_memory = max_memory;
  sandbox_settings.max_cpu_time = 1;
  if (sandbox_init(&sandbox_settings)) return 1;

  if (luajit_wrapper_load_and_run(0)) { // stdin
    return 1;
  }

  return 0;
}
