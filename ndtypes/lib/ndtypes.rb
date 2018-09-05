require 'ndtypes/errors'

require "ruby_ndtypes.so"

class String
  def b
    self.force_encoding Encoding::US_ASCII
  end
end

class NDTypes
  def == other
    if other.is_a? self.class
      self.to_s == other.to_s
    else
      false
    end
  end
end

NDT = NDTypes
