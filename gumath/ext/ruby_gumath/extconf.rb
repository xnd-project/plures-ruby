require 'mkmf'

dir_config "ruby_gumath", "/home/sameer/gitrepos/plures-ruby/build/include",
           "/home/sameer/gitrepos/plures-ruby/build/lib"

["ndtypes", "xnd", "gumath"].each do |lib|
  find_library(lib, nil, "/home/sameer/gitrepos/plures-ruby/build/lib/")
  have_library(lib)
end

["ndtypes.h", "xnd.h", "gumath.h", "ruby_ndtypes.h", "ruby_xnd.h"].each do |header|
  find_header(header, "/home/sameer/gitrepos/plures-ruby/build/include")
  have_header(header)
end

basenames = %w{gufunc_object functions ruby_gumath}
$objs = basenames.map { |b| "#{b}.o"   }
$srcs = basenames.map { |b| "#{b}.c" }

$CFLAGS += " -fPIC -g "
# FIXME: This is jugaad. Remove on deploy.
$libs += " -lndtypes -lxnd -lgumath "
create_makefile("ruby_gumath/ruby_gumath")
