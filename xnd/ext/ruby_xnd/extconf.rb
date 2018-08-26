require 'mkmf'

["ndtypes", "xnd", "gumath"].each do |lib|
  find_library(lib, nil, "/home/sameer/gitrepos/plures-ruby/build/lib/")
end

["xnd.h", "ndtypes.h", "gumath.h", "ruby_ndtypes.h"].each do |header|
  find_header(header, "/home/sameer/gitrepos/plures-ruby/build/include")
  have_header(header)
end

basenames = %w{gc_guard ruby_xnd}
$objs = basenames.map { |b| "#{b}.o"   }
$srcs = basenames.map { |b| "#{b}.c" }

$CFLAGS += " -g "
create_makefile("ruby_xnd/ruby_xnd")
