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

/******************************************************************************/

/******************************************************************************/
/************************* Resource Buffer Object *****************************/

typedef struct {
  ndt_meta_t *m;
} ResourceBufferObject;

/******************************************************************************/

static VALUE
NDTypes_initialize(VALUE self, VALUE type)
{
  return type;
}

void Init_ruby_ndtypes(void)
{
  VALUE cNDTypes = rb_define_class("NDTypes", rb_cObject);

  rb_define_method(cNDTypes, "initialize", NDTypes_initialize, 1);
}
