require 'spec_helper'

describe XND do
  context ".new" do
    it "creates a fixed array", focus: true do
      o = XND.new([[1,2,3], [2,3,4]])
      expect(o.type).to eq(NDTypes.new("2 * 3 * int64"))
      expect(o[0,0]).to eq(XND.new([1]))
    end
  end

  context ".empty" do
    
  end

  context ".from_buffer" do
    it "can import data from nmatrix objects" do
      
    end

    it "can import data from narray objects" do
      
    end
  end
end

