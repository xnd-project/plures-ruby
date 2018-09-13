require 'mkmf'

["ndtypes", "xnd", "gumath"].each do |lib|
  find_library(lib, nil, "/home/sameer/gitrepos/plures-ruby/build/lib/")
  have_library(lib)
end

["ndtypes.h", "xnd.h", "gumath.h", "ruby_ndtypes.h", "ruby_xnd.h"].each do |header|
  find_header(header, "/home/sameer/gitrepos/plures-ruby/build/include")
  have_header(header)
end

basenames = %w{functions ruby_gumath}
$objs = basenames.map { |b| "#{b}.o"   }
$srcs = basenames.map { |b| "#{b}.c" }

$CFLAGS += " -fPIC -g "
create_makefile("ruby_gumath/ruby_gumath")
