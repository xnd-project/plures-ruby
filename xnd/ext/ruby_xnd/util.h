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

/* Raise an error stored in $!. Clears it before raising. */
inline void
raise_error(void)
{
  VALUE exeception = rb_errinfo();

  rb_set_errinfo(Qnil);
  rb_exc_raise(exeception);
}

inline void
set_error(VALUE err, const char * msg)
{
  rb_set_errinfo(rb_exc_new2(err, msg));
}

static inline size_t
safe_downcast(int64_t size)
{
#if SIZE_MAX < INT64_MAX
    if (size > INT32_MAX) {
      set_error(rb_eSizeError,
                "sizes should never exceed INT32_MAX on 32-bit platforms.");
      return -1;
    }
#endif
    return (size_t)size;
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

static inline VALUE
array_new(int64_t size)
{
#if SIZE_MAX < INT64_MAX
    size_t n = safe_downcast(size);
    return n < 0 ? NULL : rb_ary_new2(n);
#else
    return rb_ary_new2(size);
#endif
}

#endif  /* UTIL_H */
