#include "ruby_xnd.h"

void Init_ruby_xnd(void)
{
  VALUE cRubyXND = rb_define_class("RubyXND", rb_cObject);

  char s[3];
  s[4] = 44;
}
