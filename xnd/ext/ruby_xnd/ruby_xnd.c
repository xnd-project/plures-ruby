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
#include "xnd.h"

VALUE cRubyXND;
VALUE cXND;
static VALUE cRubyXND_MBlock;
static VALUE rb_eNotImplementedError;
static VALUE rb_eMallocError;
static VALUE cRubyXND_Ellipsis;
static const rb_data_type_t MemoryBlockObject_type;
static const rb_data_type_t XndObject_type;

VALUE mRubyXND_GCGuard;

/****************************************************************************/
/*                           Singletons                                     */
/****************************************************************************/
static VALUE
xnd_ellipsis(void)
{
  return rb_funcall(cRubyXND_Ellipsis, rb_intern("initialize"), 0, NULL);
}

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
               "Input length (%ld) and type length (%ld) mismatch.",
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
  VALUE type;                /* owner of the current type. lives and dies with this obj. */
  xnd_t xnd;                 /* typed view, does not own anything */
} XndObject;

#define XND(xnd_p) (&(((XndObject *)xnd_p)->xnd))
#define XND_CHECK_TYPE(xnd) (CLASS_OF(xnd) == cXND)
#define GET_XND(obj, xnd_p) do {                        \
    TypedData_Get_Struct((obj), XndObject,              \
                         &XndObject_type, (xnd_p));     \
  } while (0)
#define MAKE_XND(klass, xnd_p) TypedData_Make_Struct(klass, XndObject, \
                                                    &XndObject_type, xnd_p)
#define WRAP_XND(klass, xnd_p) TypedData_Wrap_Struct(klass, &XndObject_type, xnd_p)

/* Allocate an XndObject and return wrapped in a Ruby object. */
static VALUE
XndObject_alloc(void)
{
  XndObject *xnd;

  xnd = ZALLOC(XndObject);

  xnd->mblock = 0;
  xnd->type = 0;
  xnd->xnd.bitmap.data = NULL;
  xnd->xnd.bitmap.size = 0;
  xnd->xnd.bitmap.next = NULL;
  xnd->xnd.index = 0;
  xnd->xnd.type  = NULL;
  xnd->xnd.ptr = NULL;

  return WRAP_XND(cXND, xnd);
}

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

  xnd->mblock = 0;
  xnd->type = 0;
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

#ifdef XND_DEBUG
  assert(XND(xnd_p)->type);
  assert(XND(xnd_p)->ptr);
#endif

  return self;
}


/*************************** object properties ********************************/

/* Return the ndtypes object of this xnd object. */
static VALUE
XND_type(VALUE self)
{
  XndObject *xnd_p;

  GET_XND(self, xnd_p);

  return xnd_p->type;
}

static VALUE
_XND_value(const xnd_t * const xnd_p, const int64_t maxshape)
{
  const ndt_t * const t = xnd_p->type;

#ifdef XND_DEBUG
  assert(t);
  assert(xnd_p);
#endif

  if (!ndt_is_concrete(t)) {
    rb_raise(rb_eTypeError, "type must be concrete for returning value.");
  }

  /* bitmap access needs linear index. */
  if (xnd_is_na(xnd_p)) {
    return Qnil;
  }

  switch (t->tag) {
  case FixedDim: {
    VALUE array, v;
    int64_t shape, i;

    shape = t->FixedDim.shape;

    if (shape > maxshape) {
      shape = maxshape;
    }

    array = array_new(shape);

    for (i = 0; i < shape; i++) {
      if (i == maxshape-1) {
        rb_ary_store(array, i, xnd_ellipsis());
        break;
      }

      const xnd_t next = xnd_fixed_dim_next(xnd_p, t);
      v = _XND_value(&next, maxshape);
      rb_ary_store(array, i, v);
    }

    return array;
  }
    
  case Int64: {
    int64_t temp;
    UNPACK_SINGLE(temp, xnd_p->ptr, int64_t, t->flags);
    return LL2NUM(temp);
  }

  default: {
    rb_raise(rb_eArgError, "cannot convert this type to Array.");
  }
  }
}

/* Return the value of this xnd object. Aliased to to_a. */
static VALUE
XND_value(VALUE self)
{
  XndObject *xnd_p;

  GET_XND(self, xnd_p);

  return _XND_value(XND(xnd_p), INT64_MAX);
}

/*************************** slicing functions ********************************/

#define KEY_INDEX 1
#define KEY_FIELD 2
#define KEY_SLICE 4
#define KEY_ERROR 128

/* 
   @param src_p Pointer to the source XND object from which view is being created.
   @param x Metadata for creating the view.
 */
static VALUE
RubyXND_view_move_type(XndObject *src_p, xnd_t *x)
{
  XndObject *view_p;
  VALUE type, view;

  type = rb_ndtypes_move_subtree(src_p->type, (ndt_t *)x->type);
  view = XndObject_alloc();
  GET_XND(view, view_p);

  view_p->mblock = src_p->mblock;
  view_p->type = type;
  view_p->xnd = *x;

  rb_xnd_gc_guard_register(view_p, view_p->mblock);

  return view;
}

static uint8_t
convert_single(xnd_index_t *key, VALUE obj)
{
  if (RB_TYPE_P(obj, T_FIXNUM)) {
    int64_t i = NUM2LL(obj);

    key->tag = Index;
    key->Index = i;

    return KEY_INDEX;
  }
  else if (RB_TYPE_P(obj, T_STRING)) {
    const char *s = StringValuePtr(obj);

    key->tag = FieldName;
    key->FieldName = s;

    return KEY_FIELD;
  }
  else if (CLASS_OF(obj) == rb_cRange) {
    size_t begin = NUM2LL(rb_funcall(obj, rb_intern("begin"), 0, NULL));
    size_t end = NUM2LL(rb_funcall(obj, rb_intern("end"), 0, NULL));
    /* FIXME: As of 27 Aug. 2018 Ruby trunk implements step as a property of
       Range and XND will support it as and when it is available. Maybe for 
       now we can implement a #step iterator in a separate method.
    */
    size_t step = 1; 

    key->tag = Slice;
    key->Slice.start = begin;
    key->Slice.stop = end;
    key->Slice.step = step;

    return KEY_SLICE;
  }
  else {
    rb_raise(rb_eArgError, "wrong object specified in index.");
  }
}

static uint8_t
convert_key(xnd_index_t *indices, int *len, int argc, VALUE *argv)
{
  uint8_t flags = 0;
  VALUE x;

  if (argc > 1) {
    if (argc > NDT_MAX_DIM) {
      rb_raise(rb_eArgError, "too many indices %d.", argc);
    }

    for (size_t i = 0; i < argc; i++) {
      x = argv[i];
      flags |= convert_single(indices+i, x);
      if (flags & KEY_ERROR) {
        return KEY_ERROR;
      }
    }

    *len = argc;
    return flags;
  }

  *len = 1;
  return convert_single(indices, argv[0]);
}

/* xnd_t xnd_subscript(xnd_t *x, xnd_index_t indices[], int len, */
/*                             ndt_context_t *ctx); */

/* Implement the #[] Ruby method. */
static VALUE
XND_array_aref(int argc, VALUE *argv, VALUE self)
{
  NDT_STATIC_CONTEXT(ctx);
  xnd_index_t indices[NDT_MAX_DIM];
  xnd_t x;
  int len;
  uint8_t flags;
  XndObject *xnd_p;

  if (argc == 0) {
    rb_raise(rb_eArgError, "expected atleast one argument for #[].");
  }

  flags = convert_key(indices, &len, argc, argv);
  if (flags & KEY_ERROR) {
    rb_raise(rb_eArgError, "something is wrong with the array key.");
  }
  
  GET_XND(self, xnd_p);
  x = xnd_subscript(&xnd_p->xnd, indices, len, &ctx);
  if (x.ptr == NULL) {
    // set error as per ctx.
  }

  return RubyXND_view_move_type(xnd_p, &x);
}

/* Implementation for #== method. 

   @param other Other Ruby object to compare with.
   @return VALUE [TrueClass|FalseClass]
*/
static VALUE
XND_eqeq(VALUE self, VALUE other)
{
  NDT_STATIC_CONTEXT(ctx);
  XndObject *left_p, *right_p;
  int r;

  GET_XND(self, left_p);
  GET_XND(other, right_p);

  r = xnd_equal(XND(left_p), XND(right_p), &ctx);

  if (r == 1) {
    return Qtrue;
  }
  else {
    return Qfalse;
  }
}

/* Implement Ruby spaceship operator. */
static VALUE
XND_spaceship(VALUE self, VALUE other)
{
  rb_raise(rb_eNotImpError, "spaceship not implemented yet.");

  return Qnil;
}

static VALUE
XND_strict_equal(VALUE self, VALUE other)
{
  NDT_STATIC_CONTEXT(ctx);
  XndObject *left_p, *right_p;
  int r;

  if (!XND_CHECK_TYPE(other)) {
    rb_raise(rb_eArgError, "argument type has to be XND.");
  }
  
  GET_XND(self, left_p);
  GET_XND(other, right_p);

  r = xnd_strict_equal(XND(left_p), XND(right_p), &ctx);
  if (r < 0) {
    rb_raise(rb_eRuntimeError, "r is less than 0.");
    /* TODO: change this to ctx-specific error. */
  }

  if (r) {
    return Qtrue;
  }
  else {
    return Qfalse;
  }
}

void Init_ruby_xnd(void)
{
  /* init classes */
  cRubyXND = rb_define_class("RubyXND", rb_cObject);
  cXND = rb_define_class("XND", cRubyXND);
  cRubyXND_MBlock = rb_define_class_under(cRubyXND, "MBlock", rb_cObject);
  cRubyXND_Ellipsis = rb_define_class_under(cRubyXND, "Ellipsis", rb_cObject);
  mRubyXND_GCGuard = rb_define_module_under(cRubyXND, "GCGuard");

  /* init errors */
  rb_eNotImplementedError = rb_define_class("NotImplementedError", rb_eScriptError);
  
  /* initializers */
  rb_define_alloc_func(cRubyXND, RubyXND_allocate);
  rb_define_method(cRubyXND, "initialize", RubyXND_initialize, 2);
  
  /* instance methods */
  rb_define_method(cXND, "type", XND_type, 0);
  rb_define_method(cXND, "value", XND_value, 0);
  rb_define_method(cXND, "[]", XND_array_aref, -1);
  rb_define_method(cXND, "==", XND_eqeq, 1);
  rb_define_method(cXND, "<=>", XND_spaceship, 1);
  rb_define_method(cXND, "strict_equal", XND_strict_equal, 1);
  
  rb_xnd_init_gc_guard();
}
