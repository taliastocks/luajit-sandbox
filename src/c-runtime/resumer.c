#include "resumer.h"


/*
  function Resumer:resume(...)
    Upvalues:
      - Coroutine resume_next
    local resume_to = resume_next
    resume_next = current coroutine
    yield ..., resume_to
*/
static int cr_resume(lua_State *L) {
  int nargs = lua_gettop(L);              // stack: [args...]
  lua_pushvalue(L, lua_upvalueindex(1));  // stack: [args..., resume_to]
  lua_pushthread(lua_State *L);           // stack: [args..., resume_to, current coroutine]
  lua_replace(L, lua_upvalueindex(1));    // stack: [args..., resume_to]
  return lua_yield(L, nargs + 1);
}


int cr_new_Resumer(lua_State *L) {
  int nargs = lua_gettop(L);
  if (nargs != 1) {
    return luaL_error(L, "expected 1 argument, got %d", nargs);
  } else if (lua_iscfunction(L, 1) || !lua_isfunction(L, 1)) {
    return luaL_error(L, "expected lua function");
  }
  lua_pushcclosure(L, &cr_resume, 1);
  return 1;
}
