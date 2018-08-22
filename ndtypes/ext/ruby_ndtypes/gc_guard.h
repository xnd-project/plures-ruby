/* Header file containing various functions for GC guard table. */

#ifndef GC_GUARD_H
#define GC_GUARD_H

#include "ruby_ndtypes_internal.h"

void gc_guard_unregister(NdtObject *ndt);
void gc_guard_register(NdtObject *ndt, VALUE rbuf);
void init_gc_guard(VALUE cNDTypes);

#endif  /* GC_GUARD_H */
