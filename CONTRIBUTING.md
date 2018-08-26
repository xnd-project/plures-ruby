# Error handling mechanism in plures-ruby

## Error handling in Ruby C extensions - possible approaches

The simplest approach is to use `rb_raise` for raising errors and writing functions
and passing them as hooks to `rb_rescue` for rescuing those errors.

The main concern regarding directly raising errors is that it might leave some malloc'd
memory in the system and lead to memory leaks. However, thanks to Ruby's mark-and-sweep
GC, most allocated objects should get deallocated by themselves.

The issue with directly calling `rb_raise` inside the C extension from random C functions
is that it can become hard to trace from where exactly an error is raised simply by
looking at the error.

Therefore, the error should be a relevant error raised only in the main calling instance
method. So all other functions should simply set the error and the message that they
want the error to be associated with, and the error is actually raised by the outermost
function so that the user can properly track where exactly the error came from.

So there shuold be two kinds:
* Simple errors 
* Deeper, more complex errors.


Links:

* http://clalance.blogspot.com/2011/01/writing-ruby-extensions-in-c-part-5.html
    rb_set_errinfo(rb_exc_new2(rb_eAllocError, ))
