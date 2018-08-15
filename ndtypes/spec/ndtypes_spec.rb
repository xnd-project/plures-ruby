require 'spec_helper'

describe NDTypes do
  context ".new" do
    it "creates a basic shaped memory block" do
      o = NDTypes.new("3 * uint64")

      expect_serialize o
    end
  end
end
