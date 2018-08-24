/* File containing headers for Ruby ndtypes wrapper. 
 *
 * Author: Sameer Deshmukh (@v0dro)
*/

#ifndef RUBY_NDTYPES_H
#define RUBY_NDTYPES_H

/* Public interface for ndtypes. */

extern VALUE cNDTypes;

#define NDT_CHECK_TYPE(obj) (CLASS_OF(obj) == cNDTypes)

#endif  /* RUBY_NDTYPES_H */
