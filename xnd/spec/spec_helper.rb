require 'xnd'
require 'pry'

def expect_strict_equal x1, x2
  expect(x1.strict_equal(x2)).to eq(true)
  expect(x1).to eq(x2)
end

def expect_strict_unequal x1, x2
  expect(x1.strict_equal(x2)).to eq(false)
  expect(x1).not_to eq(x2)
end
