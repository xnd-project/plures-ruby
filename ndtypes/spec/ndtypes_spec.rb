require 'spec_helper'

describe NDTypes do
  context ".new" do
    it "creates a basic shaped memory block" do
      o = NDTypes.new("3 * uint64")

      expect_serialize o
    end
  end

  context "#serialize" do
    it "serializes the given shape" do
      s = NDTypes.new "3 * char"
      s.serialize
    end
  end

  context ".deserialize" do
    it "deserializes the NDT object" do 
      s = NDTypes.new "3 * char"
      t = s.serialize
      NDTypes.deserialize t
    end
  end
end
