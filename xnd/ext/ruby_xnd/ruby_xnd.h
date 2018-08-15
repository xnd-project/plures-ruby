/* File containing headers for Ruby XND wrapper. 
 *
 * Author: Sameer Deshmukh (@v0dro)
*/
#ifndef RUBY_XND_H
#define RUBY_XND_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ruby.h"
#include "ndtypes.h"
#include "xnd.h"

  typedef struct {
    VALUE *type;                /* type owner */
    xnd_master_t *xnd;
  } MemoryBlockObject;

  typedef struct {
    MemoryBlockObject *mblock;  /* owner of the primary type and memory block */
    VALUE *type;                /* owner of the current type (NDT object)  */
    xnd_t xnd;                  /* typed view. doesn't own anything. */
  } XndObject;

#ifdef __cplusplus
}
#endif

#endif  /* RUBY_XND_H */
