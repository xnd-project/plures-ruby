require 'mkmf'

["ndtypes", "xnd", "gumath"].each do |lib|
  find_library(lib, nil, "/home/sameer/gitrepos/plures-ruby/build/lib/")
  have_library(lib)
end

["ndtypes.h", "xnd.h", "gumath.h", "ruby_ndtypes.h"].each do |header|
  find_header(header, "/home/sameer/gitrepos/plures-ruby/build/include")
  have_header(header)
end

basenames = %w{float_pack_unpack gc_guard ruby_xnd}
$objs = basenames.map { |b| "#{b}.o"   }
$srcs = basenames.map { |b| "#{b}.c" }

$CFLAGS += " -fPIC -g -Wno-undef "
create_makefile("ruby_xnd/ruby_xnd")
