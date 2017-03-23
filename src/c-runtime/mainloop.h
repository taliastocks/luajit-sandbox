#ifndef CR_MAINLOOP_H
#define CR_MAINLOOP_H

#include "lj_headers.h"


/*
  loop_once(thread, args...)
    Resume `thread` with arguments `args` until it blocks or exits.

    If `thread` has not yet started, it must already have a Lua
    function on its stack.
*/
int cr_loop_once(lua_State *);


#endif
