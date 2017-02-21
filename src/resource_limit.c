#include "resource_limit.h"


int set_resource_limit(int resource, rlim_t value) {
  struct rlimit lim;
  lim.rlim_cur = lim.rlim_max = value;
  if (setrlimit(resource, &lim) != 0) {
    return 1;
  }
  return 0;
}
