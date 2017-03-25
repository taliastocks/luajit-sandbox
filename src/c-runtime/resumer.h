#ifndef CR_RESUMER_H
#define CR_RESUMER_H

#include "lj_headers.h"


/*
  new Resumer(lua function f)
    A Resumer is a function that, when called, pauses the current thread and resumes the thread
    from which it was last called. The first time a Resumer is called, it resumes a new thread
    created from the function passed to its constructor.

    Resumers require support from a mainloop.
*/
int cr_new_Resumer(lua_State *);


void cropen_resumer(lua_State *);


#endif
