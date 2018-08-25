cd ndtypes
rake clobber && rake compile && rspec
cd ..
cp ~/gitrepos/plures-ruby/ndtypes/ext/ruby_ndtypes/ruby_ndtypes.h ~/gitrepos/plures-ruby/build/include/
cd xnd
bundle install
rake clobber && rake compile && rspec
