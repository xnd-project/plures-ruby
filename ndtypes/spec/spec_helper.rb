require 'ndtypes'

def expect_serialize ndt
  b = ndt.serialize
  u = NDTypes.deserialize b
  expect(b).to eq(u)
end
