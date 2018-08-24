/* Main file for ndtypes ruby wrapper.
 *
 * Author: Sameer Deshmukh (@v0dro)
 */

#include "ruby_ndtypes_internal.h"

/* ---------- Interal declarations ---------- */
/* data_type_t variables. */
static const rb_data_type_t NdtObject_type;
static const rb_data_type_t ResourceBufferObject_type;

/* Class declarations. */
VALUE cNDTypes;
VALUE mNDTypes_GCGuard;
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
#define RBUF_NDT_M(rbuf_p) (ResourceBufferObject*)(rbuf_p->m)
#define MAKE_RBUF(self, rbuf_p) TypedData_Make_Struct(self, ResourceBufferObject, \
                                                      &ResourceBufferObject_type, rbuf_p)
#define WRAP_RBUF(self, rbuf_p) TypedData_Wrap_Struct(self,             \
                                                      &ResourceBufferObject_type, rbuf_p)

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
#define MAKE_NDT(self, ndt_p) TypedData_Make_Struct(self, NdtObject,    \
                                                    &NdtObject_type, ndt_p)
#define WRAP_NDT(self, ndt_p) TypedData_Wrap_Struct(self, &NdtObject_type, ndt_p)

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
  
  return MAKE_NDT(obj, ndt);
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
  
  return MAKE_NDT(self, ndt);
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

#define NDTYPES_BOOL_FUNC(NDTFUNC)                                      \
  static VALUE                                                          \
  NDTypes_##NDTFUNC(VALUE self)                                         \
  {                                                                     \
    NdtObject *ndt_p;                                                   \
    GET_NDT(self, ndt_p);                                               \
    if (NDTFUNC(NDT(ndt_p))) {                                          \
      return Qtrue;                                                     \
    }                                                                   \
    return Qfalse;                                                      \
  }

NDTYPES_BOOL_FUNC(ndt_is_abstract)
NDTYPES_BOOL_FUNC(ndt_is_concrete)

NDTYPES_BOOL_FUNC(ndt_is_optional)
NDTYPES_BOOL_FUNC(ndt_is_scalar)
NDTYPES_BOOL_FUNC(ndt_is_signed)
NDTYPES_BOOL_FUNC(ndt_is_unsigned)
NDTYPES_BOOL_FUNC(ndt_is_float)
NDTYPES_BOOL_FUNC(ndt_is_complex)

NDTYPES_BOOL_FUNC(ndt_is_c_contiguous)
NDTYPES_BOOL_FUNC(ndt_is_f_contiguous)

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

/* String representation of the type. */
static VALUE
NDTypes_to_s(VALUE self)
{
  NDT_STATIC_CONTEXT(ctx);
  char *cp;
  NdtObject *ndt;
  VALUE str;

  GET_NDT(self, ndt);
  cp = ndt_as_string(NDT(ndt), &ctx);
  if (cp == NULL) {
    
  }

  str = rb_str_new_cstr(cp);
  ndt_free(cp);

  return str;
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
    /* TODO: raise error for < 0 */
  }

  str = rb_str_new(bytes, size);
  ndt_free(bytes);

  return str;
}

static VALUE
NDTypes_concrete_qmark(VALUE self)
{
  
}

/****************************************************************************/
/*                                  Class methods                           */
/****************************************************************************/

/* Deserialize a byte string into an NDTypes object. */
static VALUE
NDTypes_s_deserialize(VALUE klass, VALUE str)
{
  NdtObject *ndt_p;
  ResourceBufferObject *rbuf_p;
  VALUE ndt, rbuf;
  char *cp;
  int64_t len;
  NDT_STATIC_CONTEXT(ctx);

  Check_Type(str, T_STRING);

  cp = StringValuePtr(str);
  if (cp == NULL) {
    /* TODO: cannot get string pointer. */
  }
  len = RSTRING_LEN(str);

  rbuf_p = ALLOC(ResourceBufferObject);
  rbuf_p->m = ndt_meta_new(&ctx);
  if (rbuf_p->m == NULL) {
    /* TODO: cannot alloc meta data */
  }

  ndt_p = ALLOC(NdtObject);
  NDT(ndt_p) = ndt_deserialize(RBUF_NDT_M(rbuf_p), cp, len, &ctx);
  if (NDT(ndt_p) == NULL) {
    /* TODO: raise error for cannot deserialize */
  }

  rbuf = WRAP_RBUF(cNDTypes_RBuf, rbuf_p);
  RBUF(ndt_p) = rbuf;
  gc_guard_register(ndt_p, rbuf);
  ndt = WRAP_NDT(cNDTypes, ndt_p);
  
  return ndt;
}

/* Create a typedef */
static VALUE
NDTypes_s_typedef(VALUE klass, VALUE new_type, VALUE old_type)
{
  NDT_STATIC_CONTEXT(ctx);
  const char *cname, *ctype;
  ndt_t *t;

  Check_Type(new_type, T_STRING);
  Check_Type(old_type, T_STRING);

  cname = StringValueCStr(new_type);
  if (cname == NULL) {
    
  }

  ctype = StringValueCStr(old_type);
  if (ctype == NULL) {
    
  }

  t = ndt_from_string(ctype, &ctx);
  if (t == NULL) {
    
  }

  if (ndt_typedef(cname, t, NULL, &ctx) < 0) {
    
  }

  return Qnil;
}

static VALUE
NDTypes_s_instantiate(VALUE klass, VALUE typdef, VALUE ndt)
{
  
}

void Init_ruby_ndtypes(void)
{
  NDT_STATIC_CONTEXT(ctx);

  /* initialize NDT internals */
  ndt_init(&ctx);
  
  cNDTypes = rb_define_class("NDTypes", rb_cObject);
  cNDTypes_RBuf = rb_define_class_under(cNDTypes, "RBuf", rb_cObject);
  mNDTypes_GCGuard = rb_define_module_under(cNDTypes, "GCGuard");

  /* Initializers */
  rb_define_alloc_func(cNDTypes, NDTypes_allocate);
  rb_define_method(cNDTypes, "initialize", NDTypes_initialize, 1);

  /* Instance methods */
  rb_define_method(cNDTypes, "serialize", NDTypes_serialize, 0);
  rb_define_method(cNDTypes, "to_s", NDTypes_to_s, 0);

  /* Boolean functions */
  rb_define_method(cNDTypes, "concrete?", NDTypes_ndt_is_concrete, 0);
  rb_define_method(cNDTypes, "abstract?", NDTypes_ndt_is_abstract, 0);
  rb_define_method(cNDTypes, "optional?", NDTypes_ndt_is_optional, 0);
  rb_define_method(cNDTypes, "scalar?", NDTypes_ndt_is_scalar, 0);
  rb_define_method(cNDTypes, "signed?", NDTypes_ndt_is_signed, 0);
  rb_define_method(cNDTypes, "unsigned?", NDTypes_ndt_is_unsigned, 0);
  rb_define_method(cNDTypes, "float?", NDTypes_ndt_is_float, 0);
  rb_define_method(cNDTypes, "complex?", NDTypes_ndt_is_complex, 0);
  rb_define_method(cNDTypes, "c_contiguous?", NDTypes_ndt_is_c_contiguous, 0);
  rb_define_method(cNDTypes, "f_contiguous?", NDTypes_ndt_is_f_contiguous, 0);

  /* Class methods */
  rb_define_singleton_method(cNDTypes, "deserialize", NDTypes_s_deserialize, 1);
  rb_define_singleton_method(cNDTypes, "typedef", NDTypes_s_typedef, 2);
  rb_define_singleton_method(cNDTypes, "instantiate", NDTypes_s_instantiate, 2);

  /* Constants */
  rb_define_const(cNDTypes, "MAX_DIM", INT2NUM(NDT_MAX_DIM));

  /* GC guard init */
  init_gc_guard();
}
