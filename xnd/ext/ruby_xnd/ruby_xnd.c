#include "ruby_xnd.h"

void Init_ruby_xnd(void)
{
  VALUE xnd = rb_define_class("XND", rb_cObject);

  char s[3];
  s[4] = 44;
}
