#include "resumer.h"


/*
  Resumer:outer_loop(partial(thread, args...))
    Resume `thread` with arguments `args` until it yields to the main
    thread or exits. Should only be called from the main thread.

    The `thread` already has `args` pushed onto its stack.
    If `thread` has not yet started, it must already have a Lua
    function on its stack, before `args`.
*/
static int cr_Resumer_outer_loop(lua_State *L, lua_State *thread, int nargs) {
  // stack: [thread]
  // thread stack: [args...]
  int status;

  while (1) {
    status = lua_resume(thread, nargs);
    nargs = lua_gettop(thread); // this is the number of returned values

    if (status == 0) {
      // Thread exited without errors. Return whatever it returned.
      // stack: [thread]
      // thread stack: [args...]
      luaL_checkstack(L, nargs, "Resumer:outer_loop() could not extend stack");
      lua_xmove(thread, L, nargs);
      // stack: [thread, args...]
      return nargs; // return args...

    } else if (status == LUA_YIELD) {
      if (nargs == 0) {
        return luaL_error(L, "Resumer:outer_loop() got an unexpected yield without a thread to resume");
      }
      // Thread yielded, but may wish us to resume a different thread, passed as the last yielded value.
      // stack: [thread]
      // thread stack: [args..., next_thread or nil]
      lua_State *next_thread = NULL;
      next_thread = lua_tothread(thread, -1);
      nargs = nargs - 1; // One "argument" is actually the thread we want to resume (or nil for the main thread).
      if (next_thread != NULL) {
        // We need to resume another thread.
        // stack: [thread]
        // thread stack: [args..., next_thread]

        // There's some very careful juggling going on here. We never want to get to a state
        // where `new_thread` is unreachable from L, but we do want to break the reference from
        // L to `thread` once we no longer need it (but not before). We also need to get
        // `next_thread` off `thread`'s stack so that the arguments can be transfered onto
        // `next_thread`'s stack.
        lua_xmove(thread, L, 1); // Move `next_thread` from `thread`'s stack onto L's stack.
        // Both threads are directly reachable from L.
        // stack: [thread, next_thread]
        // thread stack: [args...]
        // next_thread stack: []

        // Avoid allocating extra stack space we don't need if the next thread is the same as
        // the one that just yielded. This silly case is supported, and just causes
        // Resumer:resume() to return its arguments.
        if (thread != next_thread) {
          luaL_checkstack(next_thread, nargs, "Resumer:outer_loop() could not extend next thread stack");
          lua_xmove(thread, next_thread, nargs); // Use return values as arguments to `next_thread`.
          // stack: [thread, next_thread]
          // thread stack: []
          // next_thread stack: [args...]
        }

        // We only cared about `thread` until we got the arguments off it. We no longer need it.
        lua_replace(L, 1); // Replace `thread` with `next_thread` as the first item on L's stack.
        // L's stack now has a direct reference to `new_thread`, and no direct reference to `thread`.
        // stack: [next_thread]
        // next_thread stack: [args...]

        thread = next_thread;

      } else {
        // We don't need to resume another thread. Return all the yielded values, except the last nil.
        lua_pop(thread, 1); // Remove the nil.
        luaL_checkstack(L, nargs, "Resumer:outer_loop() could not extend stack");
        lua_xmove(thread, L, nargs);
        return nargs;
      }

    } else {
      // Error in thread. Re-raise in the main thread.
      return luaL_error(L, "error in thread: %s", lua_tostring(thread, -1));
    }
  }
}


/*
  function Resumer:resume(...)
    Upvalues:
      - Thread resume_next
    local resume_to = resume_next
    if current thread is main thread
      resume_next = nil
      return Resumer:outer_loop(partial(resume_to, ...))
    resume_next = current thread
    return yield ..., resume_to
*/
static int cr_Resumer_resume(lua_State *L) {
  int nargs = lua_gettop(L);                // stack: [args...]
  lua_pushvalue(L, lua_upvalueindex(1));    // stack: [args..., resume_to]

  if (lua_pushthread(L)) {                  // stack: [args..., resume_to, current coroutine]
    lua_pop(L, 2);                          // stack: [args...]
    lua_State *resume_to = lua_tothread(L, lua_upvalueindex(1));
    if (resume_to == NULL) {
      // The value `resume_to` wasn't a thread, which means it must have been nil. That
      // means the caller asked to resume itself, which although silly, is supported.
      // Just return all the arguments, which is the same thing that would happen if
      // you did this in a thread other than the main thread.
      return nargs;
    }
    luaL_checkstack(resume_to, nargs, "Resumer:resume() could not extend target thread's stack");
    lua_xmove(L, resume_to, nargs);         // stack: []
    // resume_to stack: [args...]

    // Notice that we maintain a reference to `resume_to` in the upvalues until we're
    // ready to put it back in the stack, and only then do we replace the upvalue.
    // This prevents `resume_to` from becoming unreachable.
    lua_pushvalue(L, lua_upvalueindex(1));  // stack: [resume_to(args...)]
    lua_pushnil(L);                         // stack: [resume_to(args...), nil]
    lua_replace(L, lua_upvalueindex(1));    // stack: [resume_to(args...)]  ;   resume_next = nil

    // return resume(resume_to, args...)
    return cr_Resumer_outer_loop(L, resume_to, nargs);
  }

  // stack: [args..., resume_to, current coroutine]
  lua_replace(L, lua_upvalueindex(1));      // stack: [args..., resume_to]  ;   resume_next = L
  return lua_yield(L, nargs + 1);           // return: args..., resume_to
}


int cr_new_Resumer(lua_State *L) {
  int nargs = lua_gettop(L);
  if (nargs != 1) {
    return luaL_error(L, "new Resumer() expected 1 argument, got %d", nargs);
  } else if (lua_iscfunction(L, 1) || !lua_isfunction(L, 1)) {
    return luaL_error(L, "new Resumer() expected lua function");
  }
  lua_State *thread = lua_newthread(L);     // stack: [function, thread]
  lua_insert(L, -2);                        // stack: [thread, function]
  lua_xmove(L, thread, 1);                  // stack: [thread]
  // thread stack: [function]
  lua_pushcclosure(L, &cr_Resumer_resume, 1);     // stack: [resumer(thread[function])]
  return 1;
}
