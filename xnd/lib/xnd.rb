require 'ndtypes'
require "ruby_xnd.so"

require 'xnd/version'

class XND < RubyXND
  MAX_DIM = NDTypes::MAX_DIM
  
  # Methods for type inference.
  module TypeInference
    class << self
      # Infer the type of a Ruby value. In general, types should be explicitly
      # specified.
      def type_of value, dtype: nil
        NDTypes.new actual_type_of(value, dtype: dtype)
      end

      def actual_type_of
        ret = ""
        if value.is_a? Array
          data, shapes = data_shapes value
          opt = data.include? nil

          if dtype.nil?
            if data.nil?
              dtype = 'float64'
            else
              dtype = choose_dtype(data)

              data.each do |x|
                if !x.nil?
                  t = type_of(x)
                  if t != dtype
                    raise ValueError, "dtype mismatch: have t=#{t} and dtype=#{dtype}"
                  end
                end
              end
            end

            dtype = '?' + dtype if opt
          end
        elsif !dtype.nil?
          raise TypeError, "dtype arguement is only supported for Arrays."
        elsif value.is_a? Hash
          # TODO
        elsif value.nil?
          ret = '?float64'
        elsif value.is_a? Float
          ret = 'float64'
        elsif value.is_a? Complex
          ret = 'complex128'
        elsif value.is_a? Integer
          ret = 'int64'
        elsif value.is_a? String
          ret = 'string'
        else
          raise ArgumentError, "cannot infer data type for: #{value}"
        end
      end
      
      def search level, data, acc, minmax
        raise(ValueError, "too many dimensions: #{level}") if level > MAX_DIM

        current = acc[level]
        if data.nil?
          current << data
        elsif data.is_a?(Array)
          current << data.size
          next_level = level + 1
          minmax[1] = [next_level, minmax[1]].max

          if !data
            minmax[0] = [next_level, minmax[0]].min
          else
            data.each do |item|
              search level+1, item, acc, minmax
            end
          end
        else
          acc[minmax[1]] << data
          minmax[0] = [level, minmax[0]].min
        end
      end
      
      # Extract array data and dimension shapes from a nested Array. The
      # Array may contain nil for missing data or dimensions.
      #
      # @example
      # data_shapes [[0, 1], [2, 3, 4], [5, 6, 7, 8]]
      # #=> [[0, 1, 2, 3, 4, 5, 6, 7, 8], [[2, 3, 4], [3]]]
      # #              ^                    ^          ^
      # #              |                    |          `--- ndim=2: single shape 3
      # #              |                    `-- ndim=1: shapes 2, 3, 4
      # #              `--- ndim=0: extracted array data
      def data_shapes tree
        acc = Array.new(MAX_DIM + 1) { [] }
        min_level = MAX_DIM + 1
        max_level = 0
        minmax = [min_level, max_level]

        search max_level, tree, acc, minmax

        min_level = minmax[0]
        max_level = minmax[1]

        if acc[max_level] && acc[max_level].all? { |a| a.nil? }
          # min_level is not set in this special case. Hence the check.
        elsif min_level != max_level
          raise ValueError, "unbalanced tree: min depth #{min_level} and max depth #{max_level}"
        end

        data = acc[max_level]
        shapes = acc[0...max_level].reverse

        [data, shapes]
      end
    end
  end
  
  def initialize data, type: nil
    # TODO: write type inference code.
  end
end
