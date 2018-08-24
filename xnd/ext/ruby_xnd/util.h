/* Utility functions for Ruby XND wrapper. 
 * 
 * Author: Sameer Deshmukh (@v0dro)
 */

#ifndef UTIL_H
#define UTIL_H

#include "ruby.h"
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "ndtypes.h"

static inline size_t
safe_downcast(int64_t size)
{
#if SIZE_MAX < INT64_MAX
    if (size > INT32_MAX) {
        rb_raise(rb_eRuntimeError,
            "sizes should never exceed INT32_MAX on 32-bit platforms");
        return -1;
    }
#endif
    return (Py_ssize_t)size;
}

static inline bool
check_invariants(const ndt_t *t)
{
#if SIZE_MAX < INT64_MAX
    return safe_downcast(t->datasize) >= 0;
#else
    (void)t;
    return 1;
#endif
}


#endif  /* UTIL_H */
