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
static VALUE cRubyXND_Ellipsis;
static const rb_data_type_t MemoryBlockObject_type;
static const rb_data_type_t XndObject_type;

static VALUE rb_eValueError;

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

static void
_strncpy(char *dest, const void *src, size_t len, size_t size)
{
    assert (len <= size);
    memcpy(dest, src, len);
    memset(dest+len, '\0', size-len);
}

static int64_t
u8_skip_trailing_zero(const uint8_t *ptr, int64_t codepoints)
{
    int64_t i;

    for (i=codepoints-1; i >= 0; i--)
        if (ptr[i] != 0)
            return i+1;

    return 0;
}

static int64_t
u16_skip_trailing_zero(const uint16_t *ptr, int64_t codepoints)
{
    int64_t i;

    for (i=codepoints-1; i >= 0; i--)
        if (ptr[i] != 0)
            return i+1;

    return 0;
}

static int64_t
u32_skip_trailing_zero(const uint32_t *ptr, int64_t codepoints)
{
    int64_t i;

    for (i=codepoints-1; i >= 0; i--)
        if (ptr[i] != 0)
            return i+1;

    return 0;
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

static uint64_t
get_uint(VALUE data, uint64_t max)
{
  unsigned long long x;

  x = NUM2ULL(data);

  if (x > max) {
    rb_raise(rb_eArgError, "number out of range: %ulld", x);
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
      rb_raise(rb_eNotImpError,
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
    }
    return 0;
  }

  case VarDim: {
    int64_t start, step, shape;
    int64_t i;

    Check_Type(data, T_ARRAY);

    shape = ndt_var_indices(&start, &step, t, x->index, &ctx);
    if (shape < 0) {
      /* set err wrt ctx. */
    }

    if (RARRAY_LEN(data) != shape) {
      rb_raise(rb_eValueError, "expected Array with size %ld not %ld.",
               RARRAY_LEN(data), shape);
    }

    for (i = 0; i < shape; i++) {
      xnd_t next = xnd_var_dim_next(x, start, step, i);
      VALUE rb_index[1] = { LL2NUM(i) };
      
      mblock_init(&next, rb_ary_aref(1, rb_index, data));
    }

    return 0;
  }
    
  case Tuple: {
    const int64_t shape = t->Tuple.shape;
    int64_t i;

    /* since ruby does not have immutable tuple-type, we use Array instead. */
    Check_Type(data, T_ARRAY);

    if (RARRAY_LEN(data) != shape) {
      rb_raise(rb_eArgError,
               "expected Array with size %ld, not %ld.",
               shape, RARRAY_LEN(data));
    }

    for (i = 0; i < shape; i++) {
      xnd_t next = xnd_tuple_next(x, i, &ctx);
      if (next.ptr == NULL) {
        /* raise error wrt ctx. */
      }
      VALUE rb_index[1] = { LL2NUM(i) };
      
      mblock_init(&next, rb_ary_aref(1, rb_index, data));
    }

    return 0;
  }

  case Record: {
    const int64_t shape = t->Record.shape;
    VALUE temp;
    int64_t i;

    Check_Type(data, T_HASH);

    if (rb_xnd_hash_size(data) != shape) {
      rb_raise(rb_eArgError, "expected Hash size does not match with shape size.");
    }

    for (i = 0; i < shape; i++) {
      xnd_t next = xnd_record_next(x, i, &ctx);
      if (next.ptr == NULL) {
        /* error by ctx */
      }

      temp = rb_hash_aref(data, rb_str_new2(t->Record.names[i]));
      mblock_init(&next, temp);
    }

    return 0;
  }

  case Ref: {
    xnd_t next = xnd_ref_next(x, &ctx);
    if (next.ptr == NULL) {
      /* ctx error */
    }

    return mblock_init(&next, data);
  }

  case Constr: {
    xnd_t next = xnd_constr_next(x, &ctx);
    if (next.ptr == NULL) {
      
    }

    return mblock_init(&next, data);
  }

  case Nominal: {
    xnd_t next = xnd_nominal_next(x, &ctx);
    if (next.ptr == NULL) {
      /* TODO: error w.r.t ctx. */
    }

    if (t->Nominal.meth->init != NULL) {
      if (!t->Nominal.meth->init(&next, x, &ctx)) {
        rb_raise(rb_eTypeError, "could not init Nominal type in mblock_init.");
      }
      return 0;
    }

    mblock_init(&next, data);

    if (t->Nominal.meth->constraint != NULL &&
        !t->Nominal.meth->constraint(&next, &ctx)) {
      /* error by ctx. */
    }

    return 0;
  }

  case Bool: {
    int temp;
    bool b;

    if (data == Qnil) {
      rb_raise(rb_eTypeError, "assigning nil to memory block with non-optional type.");
    }

    temp = RTEST(data);
    b = (bool)temp;

    PACK_SINGLE(x->ptr, b, bool, t->flags);
    return 0;
  }
    
  case Int8: {
    int8_t temp = (int8_t)get_int(data, INT8_MIN, INT8_MAX);

    PACK_SINGLE(x->ptr, temp, int8_t, t->flags);
    return 0;
  }

  case Int16: {
    int16_t temp = (int16_t)get_int(data, INT16_MIN, INT16_MAX);

    PACK_SINGLE(x->ptr, temp, int16_t, t->flags);
    return 0;
  }

  case Int32: {
    int32_t temp = (int32_t)get_int(data, INT32_MIN, INT32_MAX);
    
    PACK_SINGLE(x->ptr, temp, int32_t, t->flags);
    return 0;
  }

  case Int64: {
    int64_t temp = get_int(data, INT64_MIN, INT64_MAX);

    PACK_SINGLE(x->ptr, temp, int64_t, t->flags);
    return 0;
  }
    
  case Uint8: {
    uint8_t temp = (uint8_t)get_uint(data, UINT8_MAX);
    PACK_SINGLE(x->ptr, temp, uint8_t, t->flags);
    return 0;
  }

  case Uint16: {
    uint16_t temp = (uint16_t)get_uint(data, UINT16_MAX);

    PACK_SINGLE(x->ptr, temp, uint16_t, t->flags);
    return 0;
  }

  case Uint32: {
    uint32_t temp = (uint32_t)get_uint(data, UINT32_MAX);
    
    PACK_SINGLE(x->ptr, temp, uint32_t, t->flags);
    return 0;
  }

  case Uint64: {
    uint64_t temp = get_uint(data, UINT64_MAX);

    PACK_SINGLE(x->ptr, temp, uint64_t, t->flags);
    return 0;
  }

  case Float16: {
    /* TODO: implement float unpacking functions. */
  }

  case Float32: {
  }

  case Float64: {
  }

  case Complex32: {
  }

  case Complex64: {
  }

  case Complex128: {
  }

  case FixedString: {
    int64_t codepoints = t->FixedString.size;
    int64_t len;

    /* FIXME: check for unicode string. */
    
    switch (t->FixedString.encoding) {
    case Ascii: {
      /* FIXME: check for ASCII string. */

      len = RSTRING_LEN(data);
      if (len > t->datasize) {
        rb_raise(rb_eValueError,
                 "maxmimum string size in bytes is %" PRIi64, codepoints);
      }

      _strncpy(x->ptr, StringValueCStr(data), (size_t)len, (size_t)t->datasize);
      return 0;
    }
      
    case Utf8: {
      /* FIXME: check for utf-8 string. */
      len = RSTRING_LEN(data);
      if (len > t->datasize) {
        rb_raise(rb_eValueError,
                 "maximum string size (in UTF-8 code points) is %" PRIi64, codepoints);
      }

      _strncpy(x->ptr, StringValueCStr(data), (size_t)len, (size_t)t->datasize);
      return 0;
    }

    case Utf16: {
      /* FIXME: create UTF-16 string from C API. */
    }

    case Utf32: {
      /* FIXME: create UTF-32 string from C API. */
    }

    case Ucs2: {
      rb_raise(rb_eNotImpError, "UCS2 encoding not implemented.");
    }

    default: {
      rb_raise(rb_eRuntimeError, "invaling string encoding.");
    }
    }
  }

  case FixedBytes: {
    int64_t size = t->FixedBytes.size;
    int64_t len;

    /* FIXME: check for bytes object. */
  }

  case String: {
    size_t size;
    const char *cp;
    char *s;

    cp = StringValueCStr(data);
    size = RSTRING_LEN(data);
    s = ndt_strdup(cp, &ctx);
    if (s == NULL) {
      /* use seterr_intx */
    }

    if (XND_POINTER_DATA(x->ptr)) {
      ndt_free(XND_POINTER_DATA(x->ptr));
    }

    XND_POINTER_DATA(x->ptr) = s;
    return 0;
  }

  case Bytes: {
  }

  case Categorical: {
  }

  case Char: {
    rb_raise(rb_eNotImpError, "'Char' type semantics need to be defined.");
  }

  case Module: {
    rb_raise(rb_eNotImpError, "'Module' type not implemented.");
  }

    /* NOT REACHED: intercepted by ndt_is_abstract(). */
  case AnyKind: case SymbolicDim: case EllipsisDim: case Typevar:
  case ScalarKind: case SignedKind: case UnsignedKind: case FloatKind:
  case ComplexKind: case FixedStringKind: case FixedBytesKind:
  case Function:
    rb_raise(rb_eArgError, "unexpected abstract type.");
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
XND_from_mblock(XndObject *xnd_p, VALUE mblock)
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

  XND_from_mblock(xnd_p, mblock);
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
_XND_value(const xnd_t * const x, const int64_t maxshape)
{
  NDT_STATIC_CONTEXT(ctx);
  const ndt_t * const t = x->type;

#ifdef XND_DEBUG
  assert(t);
  assert(x);
#endif

  if (!ndt_is_concrete(t)) {
    rb_raise(rb_eTypeError, "type must be concrete for returning value.");
  }

  /* bitmap access needs linear index. */
  if (xnd_is_na(x)) {
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

      const xnd_t next = xnd_fixed_dim_next(x, i);
      v = _XND_value(&next, maxshape);
      rb_ary_store(array, i, v);
    }

    return array;
  }

  case VarDim: {
    VALUE array, v;
    int64_t start, step, shape;
    int64_t i;

    shape = ndt_var_indices(&start, &step, t, x->index, &ctx);
    if (shape < 0) {
      /* error w.r.t ctx */
    }

    array = array_new(shape);

    for (i = 0; i < shape; i++) {
      if (i == maxshape-1) {
        rb_ary_store(array, i, xnd_ellipsis());
        break;
      }

      xnd_t next = xnd_var_dim_next(x, start, step, i);
      v = _XND_value(&next, maxshape);
      rb_ary_store(array, i, v);
    }

    return array;
  }
    
  case Tuple: {
    VALUE tuple, v;
    int64_t shape, i;

    shape = t->Tuple.shape;
    if (shape > maxshape) {
      shape = maxshape;
    }

    tuple = array_new(shape);

    for (i = 0; i < shape; i++) {
      if (i == maxshape) {
        rb_ary_store(tuple, i, xnd_ellipsis());
        break;
      }

      const xnd_t next = xnd_tuple_next(x, i, &ctx);
      if (next.ptr == NULL) {
        /* TODO: raise error wrt ctx. */
      }

      v = _XND_value(&next, maxshape);
      if (v == NULL) {
        rb_raise(rb_eValueError, "could not get tuple value in _XND_value.");
      }
      
      rb_ary_store(tuple, i, v);
    }

    return tuple;
  }
    
  case Record: {
    VALUE hash, v;
    int64_t shape, i;

    shape = t->Record.shape;
    if (shape > maxshape) {
      shape = maxshape;
    }

    hash = rb_hash_new();

    for (i = 0; i < shape; ++i) {
      if (i == maxshape - i) {
        rb_hash_aset(hash, xnd_ellipsis(), xnd_ellipsis());
        break;
      }

      xnd_t next = xnd_record_next(x, i, &ctx);
      if (next.ptr == NULL) {
        /* error wrt ctx */
      }

      v = _XND_value(&next, maxshape);
      rb_hash_aset(hash, rb_str_new2(t->Record.names[i]), v);
    }

    return hash;
  }

  case Ref: {
    xnd_t next = xnd_ref_next(x, &ctx);
    if (next.ptr == NULL) {
      /* error wr.t ctx. */
    }

    return _XND_value(&next, maxshape);
  }

  case Constr: {
    xnd_t next = xnd_constr_next(x, &ctx);
    if (next.ptr == NULL) {
      
    }

    return _XND_value(&next, maxshape);
  }

  case Nominal: {
    xnd_t next = xnd_nominal_next(x, &ctx);
    if (next.ptr == NULL) {
      
    }

    if (t->Nominal.meth->repr != NULL) {
      return t->Nominal.meth->repr(&next, &ctx);
    }

    return _XND_value(&next, maxshape);
  }

  case Bool: {
    bool temp;
    UNPACK_SINGLE(temp, x->ptr, bool, t->flags);
    return INT2BOOL(temp);
  }

  case Int8: {
    int8_t temp;
    UNPACK_SINGLE(temp, x->ptr, int8_t, t->flags);
    return INT2NUM(temp);
  }

  case Int16: {
    int16_t temp;
    UNPACK_SINGLE(temp, x->ptr, int16_t, t->flags);
    return INT2NUM(temp);
  }
    
  case Int32: {
    int32_t temp;
    UNPACK_SINGLE(temp, x->ptr, int32_t, t->flags);
    return INT2NUM(temp);
  }
    
  case Int64: {
    int64_t temp;
    UNPACK_SINGLE(temp, x->ptr, int64_t, t->flags);
    return LL2NUM(temp);
  }

  case Uint8: {
    uint8_t temp;
    UNPACK_SINGLE(temp, x->ptr, uint8_t, t->flags);
    return UINT2NUM(temp);
  }

  case Uint16: {
    uint16_t temp;
    UNPACK_SINGLE(temp, x->ptr, uint16_t, t->flags);
    return UINT2NUM(temp);
  }

  case Uint32: {
    uint32_t temp;
    UNPACK_SINGLE(temp, x->ptr, uint32_t, t->flags);
    return ULL2NUM(temp);
  }

  case Uint64: {
    uint64_t temp;
    UNPACK_SINGLE(temp, x->ptr, uint64_t, t->flags);
    return ULL2NUM(temp);
  }

  case Float16: {
    /* TODO: implement float packing functions. */
  }

  case Float32: {
  }

  case Float64: {
  }

  case Complex32: {
  }

  case Complex64: {
  }

  case Complex128: {
  }

  case FixedString: {
    int64_t codepoints = t->FixedString.size;
    
    switch (t->FixedString.encoding) {
    case Ascii: {
    }
      
    case Utf8: {
    }

    case Utf16: {
    }

    case Utf32: {
    }

    case Ucs2: {
    }

    default: {
      rb_raise(rb_eRuntimeError, "invaling string encoding.");
    } 
    }
  }

  case FixedBytes: {
  }

  case String: {
    const char *s = XND_POINTER_DATA(x->ptr);
    size_t size = s ? strlen(s) : 0;

    return rb_str_new(s, size);
  }

  case Bytes: {
    char *s = (char *)XND_BYTES_DATA(x->ptr);
    size_t size = s ? strlen(s) : 0;

    return bytes_from_string_and_size(s, size);
  }

  case Categorical: {
    int64_t k;

    UNPACK_SINGLE(k, x->ptr, int64_t, t->flags);

    switch(t->Categorical.types[k].tag) {
    case ValBool: {
      bool temp = t->Categorical.types[k].ValBool;
      return INT2BOOL(temp);
    }

    case ValInt64: {
      int64_t temp = t->Categorical.types[k].ValInt64;
      return LL2NUM(temp);
    }

    case ValFloat64: {
      double temp = t->Categorical.types[k].ValFloat64;
      return DBL2NUM(temp);
    }

    case ValString: {
      const char *temp = t->Categorical.types[k].ValString;
      return rb_str_new2(temp);
    }

    case ValNA: {
      return Qnil;
    }

    default: {
      rb_raise(rb_eRuntimeError, "unexpected category tag.");
    }
    }
  }

  case Char: {
    rb_raise(rb_eNotImpError, "char semantics need to be defined.");
  }

  case Module: {
    rb_raise(rb_eNotImpError, "'Module' type not implemented yet.");
  }

    /* NOT REACHED: intercepted by ndt_is_abstract(). */
  case AnyKind: case SymbolicDim: case EllipsisDim: case Typevar:
  case ScalarKind: case SignedKind: case UnsignedKind: case FloatKind:
  case ComplexKind: case FixedStringKind: case FixedBytesKind:
  case Function:
    rb_raise(rb_eArgError, "unexpected abstract type.");
  }

  rb_raise(rb_eRuntimeError, "invalid type tag %d.", t->tag);
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

    for (unsigned int i = 0; i < argc; i++) {
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

  if (!XND_CHECK_TYPE(other)) {
    rb_raise(rb_eArgError, "other object must be XND type.");
  }

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

/* XND#strict_equal */
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

static size_t
_XND_size(const xnd_t *x)
{
  NDT_STATIC_CONTEXT(ctx);
  const ndt_t *t = x->type;

  if (!ndt_is_concrete(t)) {
    rb_raise(rb_eTypeError, "NDT must be concrete to get size.");
  }

  if (t->ndim > 0 && ndt_is_optional(t)) {
    rb_raise(rb_eNotImpError, "optional dimensions are not supported.");
  }

  if (xnd_is_na(x)) {
    return 0;
  }

  switch (t->tag) {
  case FixedDim: {
    return safe_downcast(t->FixedDim.shape);
  }

  case VarDim: {
    int64_t start, step, shape;

    shape = ndt_var_indices(&start, &step, t, x->index, &ctx);
    if (shape < 0) {
      /* error. */
    }

    return safe_downcast(shape);
  }

  case Tuple: {
    return safe_downcast(t->Tuple.shape);
  }

  case Record: {
    return safe_downcast(t->Record.shape);
  }

  case Ref: {
    const xnd_t next = xnd_ref_next(x, &ctx);
    if (next.ptr == NULL) {
      /* error */
    }

    return _XND_size(&next);
  }

  case Constr: {
    const xnd_t next = xnd_constr_next(x, &ctx);
    if (next.ptr == NULL) {
      /* error */
    }

    return _XND_size(&next);
  }

  default: {
    rb_raise(rb_eTypeError, "type has no size.");
  }
  }  
}

/* Implement XND#size. */
static VALUE
XND_size(VALUE self)
{
  XndObject *self_p;
    
  GET_XND(self, self_p);
  return ULL2NUM(_XND_size(XND(self_p)));
}

/*************************** Singleton methods ********************************/

static VALUE
XND_s_empty(VALUE klass, VALUE type)
{
  XndObject *self_p;
  VALUE self, mblock;

  self = XndObject_alloc();
  GET_XND(self, self_p);
  
  type = rb_ndtypes_from_object(type);
  mblock = mblock_empty(type);

  XND_from_mblock(self_p, mblock);
  rb_xnd_gc_guard_register(self_p, mblock);

  return self;
}

/*************************** C-API ********************************/

size_t
rb_xnd_hash_size(VALUE hash)
{
  Check_Type(hash, T_HASH);

  return NUM2ULL(rb_funcall(hash, rb_intern("size"), 0, NULL));
}

void Init_ruby_xnd(void)
{
  /* init classes */
  cRubyXND = rb_define_class("RubyXND", rb_cObject);
  cXND = rb_define_class("XND", cRubyXND);
  cRubyXND_MBlock = rb_define_class_under(cRubyXND, "MBlock", rb_cObject);
  cRubyXND_Ellipsis = rb_define_class_under(cRubyXND, "Ellipsis", rb_cObject);
  mRubyXND_GCGuard = rb_define_module_under(cRubyXND, "GCGuard");

  /* errors */
  rb_eValueError = rb_define_class("ValueError", rb_eRuntimeError);
  
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
  rb_define_method(cXND, "size", XND_size, 0);

  /* singleton methods */
  rb_define_singleton_method(cXND, "empty", XND_s_empty, 1);
  
  rb_xnd_init_gc_guard();

#ifdef XND_DEBUG
  test_pack_float32();
  test_unpack_float32();
  test_pack_float64();
  test_unpack_float64();
#endif
}
