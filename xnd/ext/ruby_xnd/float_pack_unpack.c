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

/* Functions for packing and unpacking floats from char * arrays. 

   Author: Sameer Deshmukh (@v0dro)
*/

#include "ruby_xnd_internal.h"

#ifdef WORDS_BIGENDIAN
  #define IEEE_BIG_ENDIAN_P 1
#else
  #define IEEE_LITTLE_ENDIAN_P 1  
#endif

/* Pack a 32-bit float into a contiguous unsigned char* buffer. 
   
   Reference:
   https://github.com/python/cpython/blob/master/Include/floatobject.h#L77

   @param num The number to be packed.
   @param ptr
   @param le Le is a boolean argument. True if you want the string in
   litte-endian format, false if you want it in big-endian format.
   
   @return 0 if success. non-zero if failure.
*/
int
rb_xnd_pack_float32(double num, unsigned char* ptr, int le)
{
  /* determine endian-ness of floating point numbers. */
  /* check if the format is known */
  /* if not known do some crazy stuff and find out. */
  /* if known, simply use litte/endian defaults and loop over it. */
}

/* Unpack a 32-bit float from a contiguos unsigned char* buffer. 
  
   @param ptr : 
   @param le Le is a boolean argument. True if you want the string in
   litte-endian format, false if you want it in big-endian format.
   

   @return unpacked number as a double.
*/
double
rb_xnd_unpack_float32(unsigned char* ptr, int le)
{
  
}

#ifdef XND_DEBUG
/* Functions for testing packing/unpacking functions. 

   In order to avoid the need for injecting the dependency of a C testing framework,
   these are tests that work with the pack/unpack functions and are called in the
   Init_ function if XND_DEBUG is defined.
*/
void test_pack_float32(void)
{
  double num = 16448.0;
  int i;
  unsigned char ptr[4];

  /* test big endian */
  unsigned char ans_bige[4] = {0x00, 0x00, 0x40, 0x40};

  rb_xnd_pack_float32(num, ptr, 0);
  for (i = 0; i < 4; i++) {
    assert(ans_bige[i] == ptr[i]);
  }

  /* test little endian */
  
  unsigned char ans_lite[4] = {0x40, 0x40, 0x00, 0x00};

  rb_xnd_pack_float32(num, ptr, 1);
  for (i = 0; i < 4; i++) {
    assert(ans_lite[i] == ptr[i]);
  }
}

void test_unpack_float32(void)
{
  double answer = 16448.0;
  
  /* test big endian */
  unsigned char ptr_bige[4] = {0x00, 0x00, 0x40, 0x40};

  assert(answer == rb_xnd_unpack_float32(ptr_bige, 0));
  
  /* test little endian */
  unsigned char ptr_lite[4] = {0x40, 0x40, 0x00, 0x00};

  assert(answer == rb_xnd_unpack_float32(ptr_lite, 1));
}

void test_pack_float64(void)
{
  double num = 1090453616.0;
  int i;
  unsigned char ptr[8];

  /* test big endian. */
  unsigned char ans_bige[8] = {0x00, 0x00, 0x00, 0x00, 0x40, 0xFF, 0x00, 0x70};

  rb_xnd_pack_float64(num, ptr, 0);
  for (i = 0; i < 8; i++) {
    assert(ans_bige[i] == ptr[i]);
  }

  /* test big endian. */
  unsigned char ans_lite[8] = {0x40, 0xFF, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00};

  rb_xnd_pack_float64(num, ptr, 1);
  for (i = 0; i < 8; i++) {
    assert(ans_lite[i] == ptr[i]);
  }
}

void test_unpack_float64(void)
{
  double answer = 1090453616.0;

  /* test big-endian */
  unsigned char ptr_bige[8] = {0x00, 0x00, 0x00, 0x00, 0x40, 0xFF, 0x00, 0x70};

  assert(answer == rb_xnd_pack_float64(ptr_bige, 0));

  /* test little-endian */
  unsigned char ptr_lite[8] = {0x40, 0xFF, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00};

  assert(answer == rb_xnd_pack_float64(ptr_lite, 1));
}
#endif
