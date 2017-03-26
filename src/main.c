/*
  Entrypoint.
  Copyright (C) 2017 Collin RM Stocks. All rights reserved.
*/

#define _GNU_SOURCE

#include "luajit_wrapper.h"
#include "sandbox.h"

#include <argp.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


extern char **environ;


#define ERR_TO_STDOUT (1000)


const char *argp_program_version = "luajit-sandbox 0.0.0 (development)";
const char *argp_program_bug_address = "https://github.com/collinstocks/luajit-sandbox/issues";
static char doc[] = "Sandbox for running untrusted Lua code on Linux.";
static char args_doc[] = "[program-file]";
static struct argp_option options[] = {
  {
    "cpu", 't', "seconds", 0,
    "Maximum CPU time in seconds. The script will be notified when its time expires, "
    "and will be allowed one additional CPU second to clean up and exit gracefully. "
    "If the script does not exit by this hard limit, it is brutally killed. "
    "Zero means unlimited.\n"
    "Default: cpu=1",
    0
  },
  {
    "memory", 'm', "value", 0,
    "Maximum amount of memory available to the script in MiB. "
    "Zero means unlimited.\n"
    "Default: memory=50",
    0
  },
  {
    "err-to-stdout", ERR_TO_STDOUT, 0, 0,
    "Errors should be printed to stdout instead of stderr. "
    "Internally redirects stderr to stdout.",
    0
  },
  {
    0, 0, 0, OPTION_DOC,
    "When initializing the sandbox, open file descriptors are not closed. This means they "
    "can optionally be used to communicate with the sandboxed code. However, if you're not "
    "careful, this can be a security vulnerability. Please be careful.",
    0
  },
  { 0 }
};


struct args_struct {
  struct sandbox_settings sandbox_settings;
  char *script_file;
  bool err_to_stdout;
};


static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct args_struct *args = state->input;
  unsigned long parsed_value;
  char *end;
  errno = 0;
  switch (key) {
    case 't':
      parsed_value = strtoul(arg, &end, 10);
      if (arg == end || *end != '\0' || errno == ERANGE) {
        argp_error(state, "invalid value for -t: %s", arg);
        return EINVAL;
      }
      args->sandbox_settings.max_cpu_time = parsed_value;
      break;
    case 'm':
      parsed_value = strtoul(arg, &end, 10);
      if (arg == end || *end != '\0' || errno == ERANGE) {
        argp_error(state, "invalid value for -m: %s", arg);
        return EINVAL;
      }
      args->sandbox_settings.max_memory = parsed_value << 20;
      break;
    case ERR_TO_STDOUT:
      args->err_to_stdout = true;
      break;
    case ARGP_KEY_ARG:
      if (args->script_file == NULL) { // Only allow setting the script file once.
        args->script_file = arg;
        return 0;
      }
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };


int main(int argc, char **argv) {
  // If this process was run with environment variables, clear them all
  // by loading the process image again.
  if (environ[0] != NULL) {
    char *new_environ[] = {NULL};
    execve("/proc/self/exe", argv, new_environ);
    // If execve() returned, it failed.
    perror("failed to restart process without environment");
    return 1;
  }

  // Set defaults and parse args.
  struct args_struct args;
  args.sandbox_settings.max_memory = ((size_t) 50) << 20;
  args.sandbox_settings.max_cpu_time = 1;
  args.script_file = NULL;
  args.err_to_stdout = false;
  argp_parse(&argp, argc, argv, 0, 0, &args);

  // Redirect stderr to stdout if requested.
  if (args.err_to_stdout) {
    if (dup2(1, 2) == -1) {
      perror("failed to redirect stderr to stdout");
      return 1;
    }
  }

  // If there's a positional file argument, open it. Otherwise, use stdin.
  int progfile = 0;
  if (args.script_file != NULL) {
    progfile = open(args.script_file, O_CLOEXEC, O_RDONLY);
    if (progfile == -1) {
      perror("failed to open file");
      return 1;
    }
  }

  // Set up sandbox.
  if (sandbox_init(&args.sandbox_settings)) return 1;

  if (luajit_wrapper_load_and_run(progfile)) {
    return 1;
  }

  return 0;
}
