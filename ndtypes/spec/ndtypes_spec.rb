require 'spec_helper'

describe NDTypes do
  context ".new" do
    it "creates a basic shaped memory block" do
      o = NDTypes.new("3 * uint64")
    end
  end
end
