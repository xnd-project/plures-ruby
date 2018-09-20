require 'mkmf'

$INSTALLFILES = [
  ["ruby_ndtypes.h", "$(archdir)"]
]

["ndtypes", "xnd", "gumath"].each do |lib|
  find_library(lib, nil, "/home/sameer/gitrepos/plures-ruby/build/lib/")
end

["xnd.h", "ndtypes.h", "gumath.h"].each do |header|
  find_header(header, "/home/sameer/gitrepos/plures-ruby/build/include")
  have_header(header)
end

basenames = %w{gc_guard ruby_ndtypes}
$objs = basenames.map { |b| "#{b}.o"   }
$srcs = basenames.map { |b| "#{b}.c" }

$CFLAGS += " -O0 -g "
create_makefile("ruby_ndtypes/ruby_ndtypes")

