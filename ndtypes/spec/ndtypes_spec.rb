require 'spec_helper'

describe NDTypes do
  context "::MAX_DIM" do
    it "returns the maximum dimension possible" do
      expect(NDTypes::MAX_DIM > 0).to eq(true)
    end
  end

  context ".typedef" do
    it "creates a typedef for an official type" do
      NDTypes.typedef "node", "int32"

      t = NDTypes.new "4 * node"
      u = NDTypes.deserialize t.serialize

      expect(t).to eq(u)
    end
  end

  context ".instantiate" do
    it "instantiates NDTypes object from typedef and type" do
      NDT.typedef "node", "int32"
      NDT.typedef "cost", "int32"
      NDT.typedef "graph", "var * var * (node, cost)"

      t = NDT.new "var(offsets=[0,2]) * var(offsets=[0,3,10]) * (node, cost)"
      u = NDT.instantiate "graph", t
      expect(u.concrete?).to eq(true)

      t = NDT.new "var(offsets=[0,2]) * var(offsets=[0,2,3]) * var(offsets=[0,1,2,3]) * (node, cost)"
      expect {
        NDT.instantiate("graph", t)
      }.to raise_error(ValueError)
    end
  end
  
  context ".new" do
    it "creates a basic shaped memory block" do
      o = NDTypes.new("3 * uint64")

      expect_serialize o
    end

    context "FixedString" do
      it "checks predicates" do
        t = NDT.new "fixed_string(380, 'utf16')"

        expect_serialize t

        expect(t.abstract?).to eq(false)
        expect(t.complex?).to eq(false)
        expect(t.concrete?).to eq(true)
        expect(t.float?).to eq(false)
        expect(t.optional?).to eq(false)
        expect(t.scalar?).to eq(true)
        expect(t.signed?).to eq(false)
        expect(t.unsigned?).to eq(false)

        expect(t.c_contiguous?).to eq(true)
        expect(t.f_contiguous?).to eq(true)
      end

      it "checks fixed string common fields" do
        [
          ['ascii', 1],
          ['utf8', 1],
          ['utf16', 2],
          ['utf32', 4]
        ].each do |encoding, codepoint_size|
          t = NDT.new "fixed_string(20, '#{encoding}')"
          
          expect_serialize t
          expect(t.ndim).to eq(0)

          if RUBY_PLATFORM != "i686-linux"
            expect(t.itemsize).to eq(20 * codepoint_size)
            expect(t.align).to eq(codepoint_size)
          end

          # raises tests.
        end
      end
    end
  end

  context "#serialize" do
    it "serializes the given shape" do
      s = NDTypes.new "3 * char"
      s.serialize
    end
  end

  context "#to_s" do
    it "converts to string using ndt_as_string" do
      t = NDTypes.new "5 * int64"

      expect(t.to_s).to eq("5 * int64")
    end
  end

  context ".deserialize" do
    it "deserializes the NDT object" do 
      s = NDTypes.new "3 * char"
      t = s.serialize
      u = NDTypes.deserialize t
      
      expect(s).to eq(u)
    end
  end

  context "#abstract?" do
    it "checks for abstract type" do
      
    end
  end

  context "#concrete?" do
    it "checks for concrete type" do
      
    end
  end
end
