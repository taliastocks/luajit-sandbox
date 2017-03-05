#define _DEFAULT_SOURCE
#include <stdlib.h>

#include "luajit_wrapper.h"

#include <luajit-2.0/lua.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>

#include <unistd.h>


static int run(lua_State *L) {
  int error = lua_pcall(L, 0, 0, -2);
  if (error == LUA_ERRRUN) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    return 1;
  } else if (error == LUA_ERRMEM) {
    fprintf(stderr, "failed to allocate memory\n");
    return 1;
  } else if (error == LUA_ERRERR) {
    fprintf(stderr, "error running error handler\n");
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    return 1;
  }
  return 0;
}


struct fd_reader_closure {
  int fd;
  const char buffer[4096];
};


static const char *fd_reader(lua_State *L, void *data, size_t *size) {
  (void) L;
  struct fd_reader_closure *closure = (struct fd_reader_closure *) data;
  *size = read(closure->fd, (void *) closure->buffer, sizeof closure->buffer);
  return closure->buffer;
}


static int load_fd(lua_State *L, int fd) {
  struct fd_reader_closure closure;
  closure.fd = fd;
  int error = lua_load(L, fd_reader, &closure, "script");
  if (error == LUA_ERRSYNTAX) {
    fprintf(stderr, "syntax error\n");
    return 1;
  } else if (error == LUA_ERRMEM) {
    fprintf(stderr, "failed to allocate memory\n");
    return 1;
  }
  return 0;
}


static void initialize_vm(lua_State *L) {
  // Don't bother checking for errors in putenv() here. It can only possibly fail with ENOMEM
  // and if we've already run out of memory at this point, there are bigger problems.
  putenv("LUA_PATH="); // disable require() search path for lua files
  putenv("LUA_CPATH="); // disable require() search path for shared objects
  luaL_openlibs(L);
  lua_getglobal(L, "debug"); // stack: debug
  lua_getfield(L, -1, "traceback"); // stack: debug, debug.traceback
  lua_remove(L, -2); // stack: debug.traceback
}


int luajit_wrapper_load_and_run(int fd) {
  // Initialize the Lua state.
  int error;
  lua_State *L;
  L = luaL_newstate();
  if (L == NULL) {
    fprintf(stderr, "failed to allocate memory\n");
    return 1;
  }
  initialize_vm(L);
  error = load_fd(L, fd) || run(L);
  lua_close(L);
  return error;
}
