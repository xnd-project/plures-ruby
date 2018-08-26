/* Header file containing various functions for GC guard table. */

#ifndef GC_GUARD_H
#define GC_GUARD_H

#include "ruby_xnd_internal.h"

void gc_guard_unregister(XndObject *xnd);
void gc_guard_register(XndObject *xnd, VALUE mblock);
void init_gc_guard(VALUE);

#endif  /* GC_GUARD_H */
