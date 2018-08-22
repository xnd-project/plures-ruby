require 'spec_helper'

describe XND::TypeInference do
  context "#infer" do
    it "generates correct ndtype for fixed array" do
      value = [
        [1,2,3],
        [5,6,7]
      ]
      type = NDTypes.new "2 * 3 * int64"
      expect(XND::TypeInference.infer(value)).to eq(type)
    end

    it "generates correct ndtype for ragged array" do
      value = [
        [1,2,3],
        [5]
      ]
      type = NDTypes.new "var * var * int64"
      expect(XND::TypeInference.infer(value)).to eq(type)
    end

    it "generates correct ndtype for hash" do
      value = {
        "a" => "xyz",
        "b" => [1,2,3]
      }
      types = NDTypes.new "{a : string, b : 3 * int64}"
      expect(XND::TypeInference.infer(value)).to eq(type)
    end
  end
end
