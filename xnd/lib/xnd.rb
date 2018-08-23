require 'ndtypes'
require "ruby_xnd.so"

require 'xnd/version'

class XND < RubyXND
  # Methods for type inference.
  module TypeInference
    class << self
      # Infer the type of a Ruby value. In general, types should be explicitly
      # specified.
      def type_of value
      end

      # Extract array data and dimension shapes from a nested Array. The
      # Array may contain nil for missing data or dimensions.

      #  >>> data_shapes([[0, 1], [2, 3, 4], [5, 6, 7, 8]])
      #  ([0, 1, 2, 3, 4, 5, 6, 7, 8], [[2, 3, 4], [3]])
      #                ^                    ^       ^
      #                |                    |       `--- ndim=2: single shape 3
      #                |                    `-- ndim=1: shapes 2, 3, 4
      #                `--- ndim=0: extracted array data
      def data_shapes tree
      end
    end
  end
  
  def initialize data, type: nil
    # TODO: write type inference code.
  end
end
