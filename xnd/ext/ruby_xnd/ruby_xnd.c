/* BSD 3-Clause License
 *
 * Copyright (c) 2018, Quansight and Sameer Deshmukh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* File containing the majority implementation of the Ruby XND wrapper.
 *
 * Author: Sameer Deshmukh (@v0dro)
 */

#include "ruby_xnd_internal.h"

VALUE cRubyXND;
static VALUE cRubyXND_MBlock;
static VALUE rb_eNotImplementedError;
static VALUE rb_eMallocError;
static const rb_data_type_t MemoryBlockObject_type;

VALUE mRubyXND_GCGuard;

/****************************************************************************/
/*                           MemoryBlock Object                             */
/****************************************************************************/

/* The MemoryBlockObject is shared among several XND views/objects. */
typedef struct MemoryBlockObject {
  VALUE type;        /* type owner (ndtype) */  
  xnd_master_t *xnd; /* memblock owner */
} MemoryBlockObject;

#define GET_MBLOCK(obj, mblock_p) do {                              \
    TypedData_Get_Struct((obj), MemoryBlockObject,                  \
                         &MemoryBlockObject_type, (mblock_p));      \
  } while (0)
#define MAKE_MBLOCK(self, mblock_p) TypedData_Make_Struct(self, MemoryBlockObject, \
                                                          &MemoryBlockObject_type, mblock_p)
#define WRAP_MBLOCK(self, mblock_p) TypedData_Wrap_Struct(self,         \
                                                          &MemoryBlockObject_type, mblock_p)

/* Mark Ruby objects within MemoryBlockObject. */
static void
MemoryBlockObject_dmark(void *self)
{
  MemoryBlockObject *mblock = (MemoryBlockObject*)self;

  rb_gc_mark(mblock->type);
}

static void
MemoryBlockObject_dfree(void *self)
{
  MemoryBlockObject *mblock = (MemoryBlockObject*)self;

  xnd_del(mblock->xnd);
  mblock->xnd = NULL;
  xfree(mblock);
}

static size_t
MemoryBlockObject_dsize(const void *self)
{
  return sizeof(MemoryBlockObject);
}

static const rb_data_type_t MemoryBlockObject_type = {
  .wrap_struct_name = "MemoryBlockObject",
  .function = {
    .dmark = MemoryBlockObject_dmark,
    .dfree = MemoryBlockObject_dfree,
    .dsize = MemoryBlockObject_dsize,
    .reserved = {0,0},
  },
  .parent = 0,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

/* Allocate a MemoryBlockObject and return a pointer to allocated memory. */
static MemoryBlockObject *
mblock_alloc(void)
{
  MemoryBlockObject *self;

  self = ALLOC(MemoryBlockObject);
  if (self == NULL) {
    
  }

  self->type = NULL;
  self->xnd = NULL;

  return self;
}

/* Allocate a MemoryBlockObject and wrap it in a Ruby object. */
static VALUE
mblock_allocate(void)
{
  MemoryBlockObject *self = mblock_alloc();

  return WRAP_MBLOCK(cRubyXND_MBlock, self);
}

/* Create empty mblock with no data. */
static VALUE
mblock_empty(VALUE type)
{
  NDT_STATIC_CONTEXT(ctx);
  MemoryBlockObject *mblock_p;
  
  if (!rb_ndtypes_check_type(type)) {
    // error
  }

  mblock_p = mblock_alloc();
  mblock_p->xnd = xnd_empty_from_type(
                                      rb_ndtypes_const_ndt(type),
                                      XND_OWN_EMBEDDED, &ctx);
  if (mblock_p->xnd == NULL) {
    
  }
  mblock_p->type = type;

  return WRAP_MBLOCK(cRubyXND_MBlock, mblock_p);
}

static int64_t
get_int(VALUE data, int64_t min, int64_t max)
{
  int64_t x;

  x = NUM2LL(data);
  if (x < min || x > max) {
    rb_raise(rb_eRangeError, "Number out of range of int64 range.");
  }

  return x;
}

static int
mblock_init(xnd_t * const x, VALUE data)
{
  NDT_STATIC_CONTEXT(ctx);
  const ndt_t * const t = x->type;

  if (!check_invariants(t)) {
    rb_raise(rb_eArgError, "invariants in type.");
  }

  if (ndt_is_abstract(t)) {
    rb_raise(rb_eTypeError, "specified NDT has abstract type.");
  }

  /* set missing value. */
  if (ndt_is_optional(t)) {
    if (t->ndim > 0) {
      rb_raise(rb_eNotImplementedError,
               "optional dimensions are not implemented.");
    }

    if (data == Qnil) {
      xnd_set_na(x);
      return 0;
    }

    xnd_set_valid(x);
  }

  switch (t->tag) {
  case FixedDim: {
    const int64_t shape = t->FixedDim.shape;
    int64_t i;

    Check_Type(data, T_ARRAY);

    if (RARRAY_LEN(data) != shape) {
      rb_raise(rb_eArgError,
               "Input length (%d) and type length (%d) mismatch.",
               RARRAY_LEN(data), shape);
    }

    for (i = 0; i < shape; i++) {
      xnd_t next = xnd_fixed_dim_next(x, i);
      VALUE rb_index[1] = { LL2NUM(i) };
      
      mblock_init(&next, rb_ary_aref(1, rb_index, data));

      return 0;
    }
  }
  case Int64: {
    int64_t tmp = get_int(data, INT64_MIN, INT64_MAX);

    PACK_SINGLE(x->ptr, tmp, int64_t, t->flags);
    return 0;
  }
  default:                      /* TODO: remove after implemented all dtypes. */
    rb_raise(rb_eNotImplementedError, "invalid type tag (%d).", t->tag);
  }
}

/* Create mblock from NDT type. 
 *
 * @param type - NDT Ruby object.
 * @param data - Data as a Ruby object.
 */
static VALUE
mblock_from_typed_value(VALUE type, VALUE data)
{
  VALUE mblock;
  MemoryBlockObject *mblock_p;

  mblock = mblock_empty(type);
  GET_MBLOCK(mblock, mblock_p); 
  mblock_init(&mblock_p->xnd->master, data);

  return mblock;
}

/****************************************************************************/
/*                                 xnd object                               */
/****************************************************************************/

typedef struct XndObject {
  VALUE mblock;              /* owner of the primary type and memory block */
  VALUE type;                /* owner of the current type */
  xnd_t xnd;                 /* typed view, does not own anything */
} XndObject;

#define GET_XND(obj, xnd_p) do {                        \
    TypedData_Get_Struct((obj), XndObject,              \
                         &XndObject_type, (xnd_p));     \
  } while (0)
#define MAKE_XND(klass, xnd_p) TypedData_Make_Struct(klass, XndObject, \
                                                    &XndObject_type, xnd_p)
#define WRAP_XND(klass, xnd_p) TypedData_Wrap_Struct(klass, &XndObject_type, xnd_p)

/* Mark Ruby objects within XndObject. */
static void
XndObject_dmark(void *self)
{
  XndObject *xnd = (XndObject*)self;

  rb_gc_mark(xnd->type);
  rb_gc_mark(xnd->mblock);
}

static void
XndObject_dfree(void *self)
{
  XndObject *xnd = (XndObject*)self;

  rb_xnd_gc_guard_unregister(xnd);
  xfree(xnd);
}

static size_t
XndObject_dsize(const void *self)
{
  return sizeof(XndObject);
}

static const rb_data_type_t XndObject_type = {
  .wrap_struct_name = "XndObject",
  .function = {
    .dmark = XndObject_dmark,
    .dfree = XndObject_dfree,
    .dsize = XndObject_dsize,
    .reserved = {0,0},
  },
  .parent = 0,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void
RubyXND_from_mblock(XndObject *xnd_p, VALUE mblock)
{
  MemoryBlockObject *mblock_p;

  GET_MBLOCK(mblock, mblock_p);
 
  xnd_p->mblock = mblock;
  xnd_p->type = mblock_p->type;
  xnd_p->xnd = mblock_p->xnd->master;
}

/* Allocator for RubyXND object. Called by Ruby before initialize. */
static VALUE
RubyXND_allocate(VALUE klass)
{
  XndObject *xnd;

  xnd = ZALLOC(XndObject);

  xnd->mblock = NULL;
  xnd->type = NULL;
  xnd->xnd.bitmap.data = NULL;
  xnd->xnd.bitmap.size = 0;
  xnd->xnd.bitmap.next = NULL;
  xnd->xnd.index = 0;
  xnd->xnd.type  = NULL;
  xnd->xnd.ptr = NULL;

  return WRAP_XND(klass, xnd);
}

/* Initialize a RubyXND object. */
static VALUE
RubyXND_initialize(VALUE self, VALUE type, VALUE data)
{
  VALUE mblock;
  XndObject *xnd_p;

  mblock = mblock_from_typed_value(type, data);
  GET_XND(self, xnd_p);

  RubyXND_from_mblock(xnd_p, mblock);
  rb_xnd_gc_guard_register(xnd_p, mblock);

  return self;
}

/* Return the ndtypes object of this xnd object. */
static VALUE
RubyXND_type(VALUE self)
{
  XndObject *xnd_p;

  GET_XND(self, xnd_p);

  return xnd_p->type;
}

void Init_ruby_xnd(void)
{
  /* init classes */
  cRubyXND = rb_define_class("RubyXND", rb_cObject);
  cRubyXND_MBlock = rb_define_class_under(cRubyXND, "MBlock", rb_cObject);
  mRubyXND_GCGuard = rb_define_module_under(cRubyXND, "GCGuard");

  /* init errors */
  rb_eNotImplementedError = rb_define_class("NotImplementedError", rb_eScriptError);
  

  /* initializers */
  rb_define_alloc_func(cRubyXND, RubyXND_allocate);
  rb_define_method(cRubyXND, "initialize", RubyXND_initialize, 2);

  /* instance methods */
  rb_define_method(cRubyXND, "type", RubyXND_type, 0);

  rb_xnd_init_gc_guard();
}
