/* Main file for ndtypes ruby wrapper.
 *
 * Author: Sameer Deshmukh (@v0dro)
 */

#include "ruby_ndtypes_internal.h"

/* ---------- Interal declarations ---------- */
/* data_type_t variables. */
static const rb_data_type_t NdtObject_type;

/* Class declarations. */
VALUE cNDTypes;
static VALUE cNDTypes_RBuf;

/* ------------------------------------------ */

/******************************************************************************/
/************************* Resource Buffer Object *****************************/

/* A ResourceBufferObject is passed around various NDT objects for storing 
 * internal state. It might or might not be duplicated across other NDT
 * objects.
 */
typedef struct ResourceBufferObject {
  ndt_meta_t *m;
} ResourceBufferObject;

#define GET_RBUF(obj, rbuf_p) do {                              \
    TypedData_Get_Struct((obj), ResourceBufferObject,           \
                         &ResourceBufferObject_type, (rbuf_p)); \
} while (0)


/* GC free the ResourceBufferObject struct. */
static void
ResourceBufferObject_dfree(void * self)
{
  ResourceBufferObject * rbf = (ResourceBufferObject*)self;

  ndt_meta_del(rbf->m);
  rbf->m = NULL;
  xfree(rbf);
}

/* Calculate the size of the object. */
static size_t
ResourceBufferObject_dsize(const void *self)
{
  return sizeof(ResourceBufferObject);           
}

static const rb_data_type_t ResourceBufferObject_type = {
  .wrap_struct_name = "ResourceBufferObject",
  .function = {
    .dmark = 0,
    .dfree = ResourceBufferObject_dfree,
    .dsize = ResourceBufferObject_dsize,
    .reserved = {0,0},
  },
  .parent = 0,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE
rbuf_allocate(void)
{
  NDT_STATIC_CONTEXT(ctx);
  ResourceBufferObject *self;

  self = ALLOC(ResourceBufferObject);
  self->m = ndt_meta_new(&ctx);
  if (self->m == NULL) {
    
  }
  
  return TypedData_Make_Struct(cNDTypes_RBuf,
                               ResourceBufferObject,
                               &ResourceBufferObject_type,
                               self);
}

/******************************************************************************/

/******************************************************************************/
/************************* NDT struct object **********************************/

typedef struct NdtObject {
  VALUE rbuf;                  /* resource buffer */
  ndt_t *ndt;                   /* type */
} NdtObject;

#define NDT(v) (((NdtObject *)v)->ndt)
#define RBUF(v) (((NdtObject *)v)->rbuf)
#define GET_NDT(obj, ndt_p) do {                        \
    TypedData_Get_Struct((obj), NdtObject,              \
                         &NdtObject_type, (ndt_p));     \
  } while (0)
#define WRAP_NDT(self, ndt_p) TypedData_Make_Struct(self, NdtObject,    \
                                                    &NdtObject_type, ndt_p)

/* Get the metatdata of the ResourceBufferObject within this NDT Ruby object. */
static ndt_meta_t *
rbuf_ndt_meta(VALUE ndt)
{
  NdtObject *ndt_p;
  ResourceBufferObject *rbuf_p;
  
  GET_NDT(ndt, ndt_p);
  GET_RBUF(ndt_p->rbuf, rbuf_p);

  return rbuf_p->m;
}

/* Allocate an NDT pointer and return a Ruby object wrapped within the object. */
static VALUE
NdtObject_alloc(NdtObject *ndt)
{
  VALUE obj;
  
  WRAP_NDT(obj, ndt);
  if (obj == NULL) {
    /* TODO: raise error for no allocation. */
  }

  return obj;
}

/* GC mark the NdtObject struct. */
/* TODO: verify that there are no more object allocations happening inside ndt. */
static void
NdtObject_dmark(void * self)
{
  NdtObject * ndt = (NdtObject*)self;
  
  rb_gc_mark(ndt->rbuf);
}

/* GC free the NdtObject struct. */
static void
NdtObject_dfree(void * self)
{
  NdtObject * ndt = (NdtObject*)self;
  
  gc_guard_unregister(ndt);
  xfree(ndt);
}

/* Calculate the size of the object. */
static size_t
NdtObject_dsize(const void *self)
{
  return sizeof(NdtObject);
}

static const rb_data_type_t NdtObject_type = {
  .wrap_struct_name = "NdtObject",
  .function = {
    .dmark = NdtObject_dmark,
    .dfree = NdtObject_dfree,
    .dsize = NdtObject_dsize,
    .reserved = {0,0},
  },
  .parent = 0,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

/* Allocate an NDT object and return a Ruby object. Used for Ruby class initialization. */
static VALUE
NDTypes_allocate(VALUE self)
{
  NdtObject *ndt;
  
  return WRAP_NDT(self, ndt);
}

/* Helper function for allocating and registering NDT object GC guard hash. */
static NdtObject *
NDTypes_alloc_and_register(VALUE self)
{
  NdtObject *ndt;
  
  GET_NDT(self, ndt);
  RBUF(ndt) = rbuf_allocate();
  if (RBUF(ndt) == NULL) {
    /* check if ndt rbuf allocation was successful. raise error if not. */
  }
  gc_guard_register(ndt, RBUF(ndt));

  return ndt;
}
/******************************************************************************/


/* Initialize an instance of an NDTypes object. */
static VALUE
NDTypes_initialize(VALUE self, VALUE type)
{
  NdtObject *ndt;
  VALUE offsets = Qnil;
  const char *cp;

  /* TODO: parse kwargs with offsets. */
    
  NDT_STATIC_CONTEXT(ctx);

  Check_Type(type, T_STRING);

  cp = StringValuePtr(type);
  if (cp == NULL) {
    
  }
    
  ndt = NDTypes_alloc_and_register(self);

  NDT(ndt) = ndt_from_string_fill_meta(rbuf_ndt_meta(self), cp, &ctx);
  if (NDT(ndt) == NULL) {
    
  }

  return self;
}

/* Serialize the NDTypes object into a byte string. */
static VALUE
NDTypes_serialize(VALUE self)
{
  NdtObject *ndt;
  char *bytes;
  int64_t size;
  VALUE str;
  
  NDT_STATIC_CONTEXT(ctx);
  GET_NDT(self, ndt);

  size = ndt_serialize(&bytes, NDT(ndt), &ctx);
  if (size < 0) {
    rb_raise(rb_eArgError, "less than 0");
  }

  str = rb_str_new_cstr(bytes);
  ndt_free(bytes);

  return str;
}

/* Deserialize a byte string into an NDTypes object. */
static VALUE
NDTypes_s_deserialize(VALUE klass, VALUE str)
{
  NdtObject *ndt_p;
  VALUE ndt;
  char *cp;
  int64_t len;
  NDT_STATIC_CONTEXT(ctx);

  Check_Type(str, T_STRING);

  cp = StringValuePtr(str);
  if (cp == NULL) {
    /* TODO: cannot get string pointer. */
  }
  len = RSTRING_LEN(str);

  ndt = NdtObject_alloc(ndt_p);
  ndt_p = NDTypes_alloc_and_register(ndt);

  NDT(ndt_p) = ndt_deserialize(rbuf_ndt_meta(ndt), cp, len, &ctx);
  if (NDT(ndt_p) == NULL) {
    /* TODO: raise error for cannot deserialize */
  }

  return ndt;
}

void Init_ruby_ndtypes(void)
{
  cNDTypes = rb_define_class("NDTypes", rb_cObject);
  cNDTypes_RBuf = rb_define_class_under(cNDTypes, "RBuf", rb_cObject);

  /* Initializers */
  rb_define_alloc_func(cNDTypes, NDTypes_allocate);
  rb_define_method(cNDTypes, "initialize", NDTypes_initialize, 1);

  /* Instance methods */
  rb_define_method(cNDTypes, "serialize", NDTypes_serialize, 0);

  /* Class methods */
  rb_define_singleton_method(cNDTypes, "deserialize", NDTypes_s_deserialize, 1);

  /* GC guard table */
  init_gc_guard(cNDTypes);
}
