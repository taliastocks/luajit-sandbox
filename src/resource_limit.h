#ifndef RESOURCE_LIMIT_H
#define RESOURCE_LIMIT_H

#include <sys/resource.h>

int set_resource_limit(int, rlim_t, const char *);

#endif
