cd ndtypes
rake clobber && rake compile && rspec
cd ..
cp ~/gitrepos/plures-ruby/ndtypes/ext/ruby_ndtypes/ruby_ndtypes.h ~/gitrepos/plures-ruby/build/include/
cd xnd
bundle install
rake clobber && rake compile && rspec
cd ..
cp ~/gitrepos/plures-ruby/xnd/ext/ruby_xnd/ruby_xnd.h ~/gitrepos/plures-ruby/build/include/
cd gumath
bundle install
rake clobber && rake compile && rake test
cd ..
