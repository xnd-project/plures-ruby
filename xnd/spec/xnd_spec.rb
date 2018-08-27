require 'spec_helper'

describe XND do
  context ".new" do
    it "creates a fixed array" do
      o = XND.new([[1,2,3], [2,3,4]])
      expect(o.type).to eq(NDTypes.new("2 * 3 * int64"))
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

  context "#[]" do
    it "returns single number slice for 1D array/1 number" do
      xnd = XND.new([1,2,3,4])
      expect(xnd[1]).to eq(XND.new([2]))
    end

    it "returns single number slice for 2D array and 2 indices" do
      xnd = XND.new([[1,2,3], [4,5,6]])
      expect(xnd[0,0]).to eq(XND.new([1]))      
    end
  end

  context "#==" do
    it "returns true if elements of XND objects are equal" do
      x1 = XND.new [[1,2,3], [4,5,6]]
      x2 = XND.new [[1,2,3], [4,5,6]]

      expect(x1 == x2).to eq(true)
    end

    it "returns false if elements of XND objects are unequal" do
      x1 = XND.new [[1,2,3], [4,5,6]]
      x2 = XND.new [[1,2,3], [4,53,2]]

      expect(x1 == x2).to eq(false)      
    end
  end

  context "#to_a" do
    it "returns Ruby Array for FixedDim XND array" do
      
    end
  end

  context "#to_s" do
    it "returns String representation" do
      
    end
  end

  context "#size" do
    it "returns the size of the XND array" do
      
    end
  end
end

