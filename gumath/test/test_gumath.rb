require 'test_helper'

Fn = Gumath::Functions

class TestFunctionHash < Minitest::Test
  def test_hash_contents
    hash = Fn.instance_variable_get(:@gumath_functions)

    assert_instance_of Gumath::GufuncObject, hash[:sin]
  end
end

class TestCall < Minitest::Test
  def test_sin_scalar
    x1 = XND.new(1.2, type: "float64")
    y1 = Fn.sin(x1)
    
    x2 = XND.new(1.23e1, type: "float32")
    y2 = Fn.sin(x2)

    assert_in_delta y1, Math.sin(1.2), 0.00001
    assert_in_delta y2, Math.sin(1.23e1), 0.00001
  end

  def test_sin
    
  end
end



