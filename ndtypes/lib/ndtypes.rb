require 'ndtypes/errors'

require "ruby_ndtypes.so"

class String
  def b
    self.force_encoding Encoding::US_ASCII
  end
end

NDT = NDTypes
