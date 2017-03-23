#include "mainloop.h"


int cr_loop_once(lua_State *L) {
  int status;
  int nargs = lua_gettop(L) - 1;                    // stack: [thread, args...]

  if (nargs < 0) {
    return luaL_error(L, "cr_loop_once() expected at least 1 argument, a thread");
  }

  lua_State *thread = lua_tothread(L, 1);

  if (thread == NULL) {
    return luaL_error(L, "cr_loop_once() expected a thread as its first argument");
  }

  luaL_checkstack(thread, nargs, "cr_loop_once() could not extend thread stack");
  lua_xmove(L, thread, nargs); // move all the arguments onto the thread's stack

  while (1) {
    status = lua_resume(thread, nargs);
    nargs = lua_gettop(thread); // this is the number of returned values

    if (status == 0) {
      // Thread exited without errors. Return whatever it returned.
      luaL_checkstack(L, nargs, "cr_loop_once() could not extend stack");
      lua_xmove(thread, L, nargs);
      return nargs;

    } else if (status == LUA_YIELD) {
      // Thread yielded, but may wish us to resume a different thread, passed as the last yielded value.
      lua_State *next_thread = NULL;
      if (nargs > 0) {
        next_thread = lua_tothread(thread, -1);
      }
      if (next_thread != NULL) {
        // We need to resume another thread.
        nargs = nargs - 1; // One "argument" is actually the thread we want to resume.

        // There's some very careful juggling going on here. We never want to get to a state
        // where `new_thread` is unreachable from L, but we do want to break the reference from
        // L to `thread` once we no longer need it (but not before). We also need to get
        // `next_thread` off `thread`'s stack so that the arguments can be transfered onto
        // `next_thread`'s stack.
        lua_xmove(thread, L, 1); // Move `next_thread` from `thread`'s stack onto L's stack.
        // Both threads are directly reachable from L.

        luaL_checkstack(next_thread, nargs, "cr_loop_once() could not extend next thread stack");
        lua_xmove(thread, next_thread, nargs); // Use return values as arguments to `next_thread`.

        // We only cared about `thread` until we got the arguments off it. We no longer need it.
        lua_replace(L, 1); // Replace `thread` with `next_thread` as the first item on L's stack.
        // L's stack now has a direct reference to `new_thread`, and no direct reference to `thread`.

        thread = next_thread;

      } else {
        // We don't need to resume another thread. Return all the yielded values.
        luaL_checkstack(L, nargs, "cr_loop_once() could not extend stack");
        lua_xmove(thread, L, nargs);
        return nargs;
      }

    } else {
      // Error in thread. Re-raise in the main thread.
      return luaL_error(L, "error in thread: %s", lua_tostring(thread, -1));
    }
  }
}
