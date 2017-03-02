#include <stddef.h>
#include <dlfcn.h>


static __thread char *_dlerror = NULL;


void *dlopen(const char *filename, int flags) {
  (void) filename;
  (void) flags;
  _dlerror = "dlopen not supported";
  return NULL;
}


void *dlsym(void *handle, const char *symbol) {
  (void) handle;
  (void) symbol;
  _dlerror = "dlsym not supported";
  return NULL;
}


int dlclose(void *handle) {
  (void) handle;
  return 0;
}


char *dlerror(void) {
  char *ret = _dlerror;
  _dlerror = NULL;
  return ret;
}
