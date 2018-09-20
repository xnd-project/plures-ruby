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

## Error handling in C APIs of Ruby C extensions

None of the C APIs will ever raise a Ruby error. Error conditions are indicated by the
return value of the function. The error values are as follows:

* Success - 
* Failure - 

# Differences between Python and Ruby wrappers

## True and false

Some differences do arise between the Python and Ruby wrappers mainly due to semantic
differences between the two languages. For example, in Python `True` and `False` are
simply keywords for `1` and `0` respectively. Therefore, `if 0:` in Python will evaluate
to `false`. Therefore `False == 0` in Python evaluates to `true` in Python.

However, in Ruby, `false` and `nil` are the only real `falsey` values and everything else
is `truthy` (including `0`). `false == 0` evaluates to `false` in Ruby due to this reason.

Therefore, when creating boolean typed arrays, the result will always be in terms of `true`
and `false` in Ruby whereas in Python it can be `0` or `False` for falsey values and anything
else for truth-y.

## Slice ranges

Ruby 2.6 introduces infinite ranges, however the rubies before that rely on `Float::INFINITY`
to denote 'infinity' in a Range. To make XND work with infinite ranges, XND will treat
`Float::INFINITY` as 'infinite' in a Range until Ruby 2.6 is released.

Note that this behaviour is contrary to the behaviour of `Array`, where specfying `Float::INFINITY`
in a Range throws an error.

# GC interfacing

Both XND and NDT use 'GC guard tables' to keep Ruby object in use from getting garbage collected.
This needs to be done because some objects use internal Ruby objects that are shared between
multiple user-facing objects. 

The chain of objects in the GC table is as follows:

In NDT:
NDT -> RBUF

In XND:
XND -> MBLOCK

# Roadmap

Short term goals:

* Segregate into separate repos and release usable ruby gems.
* Sharing buffer pointers between XND and other library.
* Usage and installation docs for all extensions.
* Better file segregation for C extensions.
* Move from rspec to minitest for all extensions.
* Finish to_s and match functions in XND.
* Write Ruby kernel generator.
