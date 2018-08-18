/* Functions useful for interfacing shared rbuf objects with the Ruby GC. */
/* Author: Sameer Deshmukh (@v0dro) */
#include "gc_guard.h"

#define GC_GUARD_TABLE_NAME "__gc_guard_table"

static ID id_gc_guard_table;

/* Set the GC guard  */
void
gc_guard_aset(NdtObject *ndt, VALUE rbuf)
{
  VALUE table = rb_ivar_get(cNDTypes, id_gc_guard_table);
  rb_hash_aset(table, PTR2NUM);
}

/* Initialize the global GC guard table. klass is a VALUE reprensenting NDTypes class. */
void
init_gc_guard(VALUE klass);
{
  id_gc_guard_table = rb_intern(GC_GUARD_TABLE_NAME);
  rb_ivar_set(klass, id_gc_guard_table, rb_hash_new());
}

