#include <luajit-2.0/lua.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>

#include "sandbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


const size_t max_memory = ((size_t) 50) << 20;


static int traceback(lua_State *L) {
  if (!lua_isstring(L, 1)) { /* 'message' not a string? */
    return 1;  /* keep it intact */
  }
  lua_pushstring(L, "traceback");
  lua_gettable(L, LUA_REGISTRYINDEX);
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}


int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

  // Set up resource limits.
  struct sandbox_settings sandbox_settings;
  sandbox_settings.max_memory = max_memory;
  sandbox_settings.max_cpu_time = 1;
  if (sandbox_init(&sandbox_settings)) return 1;

  // Program.
  char *buffer = "io.write('hello world\\n')\ndebug=0\nhello.world()";
  size_t bufsize = strlen(buffer);

  // Initialize the Lua state.
  int result;
  lua_State *L;
  L = luaL_newstate();
  if (L == NULL) {
    fprintf(stderr, "failed to allocate memory");
    return 1;
  }
  luaL_openlibs(L);
  lua_pushcfunction(L, traceback);
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  lua_pushstring(L, "traceback");
  lua_getfield(L, -2, "traceback");
  lua_settable(L, LUA_REGISTRYINDEX);
  lua_pop(L, 1);
  result = luaL_loadbuffer(L, buffer, bufsize, "script");
  if (result == LUA_ERRSYNTAX) {
    fprintf(stderr, "syntax error");
    lua_close(L);
    return 1;
  } else if (result == LUA_ERRMEM) {
    fprintf(stderr, "failed to allocate memory");
    lua_close(L);
    return 1;
  }
  result = lua_pcall(L, 0, LUA_MULTRET, lua_gettop(L) - 1);
  if (result == LUA_ERRRUN) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_close(L);
    return 1;
  } else if (result == LUA_ERRMEM) {
    fprintf(stderr, "failed to allocate memory");
    lua_close(L);
    return 1;
  } else if (result == LUA_ERRERR) {
    fprintf(stderr, "error running error handler\n");
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_close(L);
    return 1;
  }
  lua_close(L);

  // Simulate misbehaving code.
  for (;;);
  /*
  for (int i; malloc(1 << 20); ++i) {
    printf("%i\n", i);
  }
  */

  return 0;
}
