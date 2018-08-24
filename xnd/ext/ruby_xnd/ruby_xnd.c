#include "ruby_xnd_internal.h"

static VALUE cRubyXND;
static VALUE cRubyXND_MBlock;
static const rb_data_type_t MemoryBlockObject_type;

/****************************************************************************/
/*                           MemoryBlock Object                             */
/****************************************************************************/

/* The MemoryBlockObject is shared among several XND views/objects. */
typedef struct {
  VALUE type;        /* type owner (ndtype) */  
  xnd_master_t *xnd; /* memblock owner */
} MemoryBlockObject;

#define GET_MBLOCK(obj, mblock_p) do {                              \
    TypedData_Get_Struct((obj), MemoryBlockObject,                  \
                         &MemoryBlockObject_type, (mblock_p));      \
} while (0)
#define MAKE_MBLOCK(self, mblock_p) TypedData_Make_Struct(self, MemoryBufferObject, \
                                                        &MemoryBufferObject_type, mblock_p)
#define WRAP_MBLOCK(self, mblock_p) TypedData_Wrap_Struct(self,             \
                                                      &MemoryBufferObject_type, mblock_p)

/* Mark Ruby objects within MemoryBufferObject. */
static void
MemoryBufferObject_dmark(void *self)
{
  MemoryBufferObject *mblock = (MemoryBufferObject*)self;

  rb_gc_mark(mblock->type);
}

static void
MemoryBufferObject_dfree(void *self)
{
  MemoryBufferObject *mblock = (MemoryBufferObject*)self;

  xnd_del(mblock->xnd);
  mblock->xnd = NULL;
  xfree(mblock);
}

static size_t
MemoryBufferObject_dsize(const void *self)
{
  return sizeof(MemoryBufferObject);
}

static const rb_data_type_t MemoryBufferObject_type = {
  .wrap_struct_name = "MemoryBufferObject",
  .function = {
    .dmark = MemoryBufferObject_dmark,
    .dfree = MemoryBufferObject_dfree,
    .dsize = MemoryBufferObject_dsize,
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
  MemoryBlockObject *mblock;
  NdtObject *ndt;
  VALUE self;
  
  if (!NDT_CHECK_TYPE(type)) {
    rb_raise(rb_eArgError, "expected NDT object.");
  }

  GET_NDT(type, ndt);

  mblock = mblock_alloc();
  mblock->xnd = xnd_empty_from_type(CONST_NDT(ndt), XND_OWN_EMBEDDED, &ctx);
  if (mblock->xnd == NULL) {
    
  }
  mblock->type = type;

  return MAKE_MBLOCK(cRubyXND_MBlock, mblock);
}

static int
mblock_init(xnd_t * const x, VALUE data)
{
  NDT_STATIC_CONTEXT(ctx);
  const ndt_t * const t = x->type;

  if (!check_invariants(t)) {
    
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
  
  if (mblock_init(mblock_p->xnd->master, data) < 0) {
    
  }

  return WRAP_MBLOCK(cRubyXND_MBlock, mblock_p);
}

/****************************************************************************/
/*                                 xnd object                               */
/****************************************************************************/

typedef struct {
  VALUE mblock;              /* owner of the primary type and memory block */
  VALUE type;                /* owner of the current type */
  xnd_t xnd;                 /* typed view, does not own anything */
} XndObject;

#define GET_XND(obj, xnd_p) do {                        \
    TypedData_Get_Struct((obj), XndObject,              \
                         &XndObject_type, (ndt_p));     \
  } while (0)
#define MAKE_XND(klass, mblock_p) TypedData_Make_Struct(klass, XndObject, \
                                                    &XndObject_type, xnd_p)
#define WRAP_XND(klass, mblock_p) TypedData_Wrap_Struct(klass, &XndObject_type, xnd_p)

static VALUE
RubyXND_from_mblock(VALUE self, VALUE mblock)
{
  
}

/* Allocator for RubyXND object. Called by Ruby before initialize. */
static VALUE
RubyXND_allocate(VALUE self)
{
  XndObject *xnd;

  xnd = ALLOC(XndObject);
  if (xnd == NULL) {
    
  }

  xnd->mblock = NULL;
  xnd->type = NULL;
  xnd->xnd.bitmap.data = NULL;
  xnd->xnd.bitmap.size = 0;
  xnd->xnd.bitmap.next = NULL;
  xnd->xnd.index = 0;
  xnd->xnd.type  = NULL;
  xnd->xnd.ptr = NULL;

  return WRAP_XND(self, xnd);
}

/* Initialize a RubyXND object. */
static VALUE
RubyXND_initialize(VALUE self, VALUE type, VALUE data)
{
  VALUE mblock;

  mblock = mblock_from_typed_value(type, data);

  return RubyXND_from_mblock(self, mblock);
}

void Init_ruby_xnd(void)
{
  cRubyXND = rb_define_class("RubyXND", rb_cObject);
  cRubyXND_MBlock = rb_define_class_under(cRubyXND, "MBlock", rb_cObject);

  /* initializers */
  rb_define_alloc_func(cRubyXND, RubyXND_allocate);
  rb_define_method(cRubyXND, "initialize", RubyXND_initialize, 2);
}
