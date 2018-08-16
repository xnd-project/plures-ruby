#include "ruby_ndtypes.h"
/******************************************************************************/
/************************* NDT struct object **********************************/

typedef struct {
  VALUE *rbuf;                  /* resource buffer */
  ndt_t *ndt;                   /* type */
} NdtObject;

#define NDT(v) (((NdtObject *)v)->ndt)
#define RBUF(v) (((NdtObject *)v)->rbuf)
#define RBUF_NDT_META(v) (((ResourceBufferObject *)(((NdtObject *)v)->rbuf))->m)

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
  xfree(ndt);
}

/* Calculate the size of the object. */
static size_t
NdtObject_dsize(const void *self)
{
  
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


static VALUE
NDTypes_allocate(VALUE self)
{
  NdtObject *ndt;
  
  return TypedData_Make_Struct(self, NdtObject, &NdtObject_type, ndt);
}

/******************************************************************************/

/******************************************************************************/
/************************* Resource Buffer Object *****************************/

typedef struct {
  ndt_meta_t *m;
} ResourceBufferObject;

/******************************************************************************/

/* Initialize an instance of an NDTypes object. */
static VALUE
NDTypes_initialize(VALUE self, VALUE type)
{
  NDT_STATIC_CONTEXT(ctx);
  /* ndtype_alloc - allocate the main ndt object */
  /* rbuf_alloc -  allocate resource buffer object. This object is never exposed
     to the user so it must be allocated internally inside some internal object.
     some ruby object allocation will happen here.*/
  /* ndt_from_string_fill_meta - function from NDT library. Gives metadata about
     the type. Keep in mind that the type still stores pointers to the metadata.*/
  return type;
}

static VALUE
NDTypes_serialize(VALUE self)
{
  
}

static VALUE
NDTypes_s_deserialize(VALUE self)
{
  
}

void Init_ruby_ndtypes(void)
{
  VALUE cNDTypes = rb_define_class("NDTypes", rb_cData);

  rb_define_alloc_func(cNDTypes, NDTypes_allocate);
  rb_define_method(cNDTypes, "initialize", NDTypes_initialize, 1);
  rb_define_method(cNDTypes, "serialize", NDTypes_serialize, 0);

  rb_define_singleton_method(cNDTypes, "deserialize", NDTypes_s_deserialize, 1);
}
