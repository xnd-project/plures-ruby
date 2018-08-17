#include "ruby_ndtypes.h"

static const rb_data_type_t NdtObject_type;

/* Class declarations. */
VALUE cNDTypes;
static VALUE cNDTypes_RBuf;

/******************************************************************************/
/************************* Resource Buffer Object *****************************/

/* A ResourceBufferObject is passed around various NDT objects for storing 
 * internal state. It might or might not be duplicated across other NDT
 * objects.
 */
typedef struct {
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

typedef struct {
  VALUE rbuf;                  /* resource buffer */
  ndt_t *ndt;                   /* type */
} NdtObject;

#define NDT(v) (((NdtObject *)v)->ndt)
#define RBUF(v) (((NdtObject *)v)->rbuf)
#define GET_NDT(obj, ndt_p) do {                        \
    TypedData_Get_Struct((obj), NdtObject,              \
                         &NdtObject_type, (ndt_p));     \
  } while (0)


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
  
  /* TODO: what do to about the rbuf VALUE pointer. How to free it? */
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
  
  return TypedData_Make_Struct(self, NdtObject, &NdtObject_type, ndt);
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

  cp = RSTRING_PTR(type);
  if (cp == NULL) {
    
  }
    
  GET_NDT(self, ndt);
  RBUF(ndt) = rbuf_allocate();
  if (RBUF(ndt) == NULL) {
    
  }

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
NDTypes_s_deserialize(VALUE tp, VALUE ndtype)
{
  NDT_STATIC_CONTEXT(ctx);
  VALUE self;

  
}

void Init_ruby_ndtypes(void)
{
  VALUE cNDTypes = rb_define_class("NDTypes", rb_cObject);
  VALUE cNDTypes_RBuf = rb_define_class_under(cNDTypes, "RBuf", rb_cObject);

  rb_define_alloc_func(cNDTypes, NDTypes_allocate);
  rb_define_method(cNDTypes, "initialize", NDTypes_initialize, 1);
  rb_define_method(cNDTypes, "serialize", NDTypes_serialize, 0);

  rb_define_singleton_method(cNDTypes, "deserialize", NDTypes_s_deserialize, 1);
}
