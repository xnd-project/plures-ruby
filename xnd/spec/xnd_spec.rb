require 'spec_helper'

describe XND do
  context ".new" do
    it "creates a basic XND object" do
      o = XND.new([[1,2,3], [2,3,4]])
      expect(o.type).to eq(NDTypes)
      expect(o[0,0]).to eq(XND.new([1]))
    end
  end
end
