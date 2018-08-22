require "ruby_ndtypes.so"

class NDTypes
  def == other
    if other.is_a? self.class
      self.to_s == other.to_s
    else
      false
    end
  end
end
