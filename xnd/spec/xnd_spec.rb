require 'spec_helper'

describe XND do
  context ".new" do
    it "creates a fixed array" do
      o = XND.new([[1,2,3], [2,3,4]])
      expect(o.type).to eq(NDTypes.new("2 * 3 * int64"))
      expect(o[0,0]).to eq(XND.new([1]))
    end

    it "accepts a type for fixed array" do
      t = NDT.new("2 * 3 * int64")
      o = XND.new([[1,2,3], [2,3,4]], type: t)

      expect(o.type).to eq(t)
    end

    it "raises ArgumentError for type and input mismatch" do
      t = NDT.new "3 * 3 * int64"
      expect {
        XND.new([[1,2,3], [2,3,4]], type: t)
      }.to raise_error(ArgumentError)
    end

    it "raises ValueError for wrong input type in int64 array" do
      t = NDT.new "2 * 3 * int64"
      expect {
        XND.new([[1,2,"peep!"], [2,3,4]], type: t)
      }.to raise_error(ValueError)      
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

