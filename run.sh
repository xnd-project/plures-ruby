cd ndtypes
rake clobber && rake compile && rspec
cd ..
cd xnd
bundle install
rake clobber && rake compile && rspec
