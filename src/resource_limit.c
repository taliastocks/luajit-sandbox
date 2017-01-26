#include "resource_limit.h"

#include <stdio.h>


int set_resource_limit(int resource, rlim_t value, const char *failure_message) {
  struct rlimit lim;
  lim.rlim_cur = lim.rlim_max = value;
  if (setrlimit(resource, &lim) != 0) {
    if (failure_message != NULL) {
      perror(failure_message);
    }
    return 1;
  }
  return 0;
}
