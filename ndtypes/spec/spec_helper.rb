require 'ndtypes'
require 'pry'

def expect_serialize t
  b = t.serialize
  u = NDTypes.deserialize b.b

  expect(u).to eq(t)
end
