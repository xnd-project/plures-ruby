require 'spec_helper'

describe XND do
  context ".new" do
    skip "Type Inference" do
      # tuple

      # record

      # float64

      # complex 128

      # int64

      context "String", focus: true do
        t = ["supererogatory", "exiguous"]
        typeof_t = "(string, string)"

        [
          [["mov"], "1 * string"],
          [["mov", "$0"], "2 * string"],
          [[["cmp"], ["$0"]], "2 * 1 * string"],

          [t, typeof_t],
          [[t] * 2, "2 * %s" % typeof_t],
          [[[t] * 2] * 10, "10 * 2 * %s" % typeof_t]
        ].each do |v, t|
          it "type: #{t}" do
            x = XND.new v

            expect(x.type).to eq(NDT.new(t))
            expect(x.value).to eq(v)            
          end
        end
      end

      context "Bytes" do
        t = ["lagrange".b, "points".b]
        typeof_t = "(bytes, bytes)"

        [
          [["L1".b], "1 * bytes"],
          [["L2".b, "L3".b, "L4".b], "3 * bytes"],
          [[["L5".b], ["none".b]], "2 * 1 * bytes"],

          [t, typeof_t],
          [[t] * 2, "2 * %s" % typeof_t],
          [[[t] * 2] * 10, "10 * 2 * %s" % typeof_t]
        ].each do |v, t|
          it "type: {t}" do
            x = XND.new v
            
            expect(x.type).to eq(NDT.new(t))
            expect(x.value).to eq(v)
          end
        end
      end

      # optional
    end
    
    context "FixedDim" do
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

      skip "raises ValueError for wrong input type in int64 array" do
        t = NDT.new "2 * 3 * int64"
        expect {
          XND.new([[1,2,"peep!"], [2,3,4]], type: t)
        }.to raise_error(ValueError)      
      end      
    end

    context "VarDim" do
      
    end

    skip "FixedString" do
      it "creates FixedString utf16" do
        t = "2 * fixed_string(3, 'utf16')"
        v = ["\u1111\u2222\u3333", "\u1112\u2223\u3334"]
        x = XND.new v, type: t
        
        expect(x.value).to eq(v)
      end

      it "creates FixedString utf32 - figure a way to specify 32bit codepoints." do
        t = "2 * fixed_string(3, 'utf32')"
        v = ["\x00\x01\x11\x11\x00\x02\x22\x22\x00\x03\x33\x33".u32,
             "\x00\x01\x11\x12\x00\x02\x22\x23\x00\x03\x33\x34".u32]
        x = XND.new v, type: t
        
        expect(x.value).to eq(v)
      end
    end
  end

  context ".empty" do
    context "FixedDim" do      
      DTYPE_EMPTY_TEST_CASES.each do |v, s|
        [
          [[v] * 0, "0 * #{s}" ],
          [[v] * 1, "1 * #{s}" ],
          [[v] * 2, "2 * #{s}" ],
          [[v] * 1000, "1000 * #{s}" ],
          
          [[[v] * 0] * 0, "0 * 0 * #{s}" ],
          [[[v] * 1] * 0, "0 * 1 * #{s}" ],
          [[[v] * 0] * 1, "1 * 0 * #{s}" ],
          
          [[[v] * 1] * 1, "1 * 1 * #{s}" ],
          [[[v] * 2] * 1, "1 * 2 * #{s}" ],
          [[[v] * 1] * 2, "2 * 1 * #{s}" ],
          [[[v] * 2] * 2, "2 * 2 * #{s}" ],
          [[[v] * 3] * 2, "2 * 3 * #{s}" ],
          [[[v] * 2] * 3, "3 * 2 * #{s}" ],
          [[[v] * 40] *3 , "3 * 40 * #{s}" ]
        ].each do |vv, ss|
          it "type: #{ss}" do
            t = NDT.new ss
            x = XND.empty ss

            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
            expect(x.size).to eq(vv.size)
          end
        end
      end      
    end

    context "VarDim" do
      DTYPE_EMPTY_TEST_CASES[0..10].each do |v, s|
        [
          [[v] * 0, "var(offsets=[0,0]) * #{s}"],
          [[v] * 1, "var(offsets=[0,1]) * #{s}"],
          [[v] * 2, "var(offsets=[0,2]) * #{s}"],
          [[v] * 1000, "var(offsets=[0,1000]) * #{s}"],
          
          [[[v] * 0] * 1, "var(offsets=[0,1]) * var(offsets=[0,0]) * #{s}"],
          
          [[[v], []], "var(offsets=[0,2]) * var(offsets=[0,1,1]) * #{s}"],
          [[[], [v]], "var(offsets=[0,2]) * var(offsets=[0,0,1]) * #{s}"],
          
          [[[v], [v]], "var(offsets=[0,2]) * var(offsets=[0,1,2]) * #{s}"],
          [[[v], [v] * 2, [v] * 5], "var(offsets=[0,3]) * var(offsets=[0,1,3,8]) * #{s}"]
        ].each do |vv, ss|
          it "type: #{ss}" do
            t = NDT.new ss
            x = XND.empty ss
            
            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
            expect(x.size).to eq(vv.size)
          end
        end
      end
    end

    context "Fortran" do
      DTYPE_EMPTY_TEST_CASES.each do |v, s|
        [
          [[v] * 0, "!0 * #{s}"],
          [[v] * 1, "!1 * #{s}"],
          [[v] * 2, "!2 * #{s}"],
          [[v] * 1000, "!1000 * #{s}"],

          [[[v] * 0] * 0, "!0 * 0 * #{s}"],
          [[[v] * 1] * 0, "!0 * 1 * #{s}"],
          [[[v] * 0] * 1, "!1 * 0 * #{s}"],

          [[[v] * 1] * 1, "!1 * 1 * #{s}"],
          [[[v] * 2] * 1, "!1 * 2 * #{s}"],
          [[[v] * 1] * 2, "!2 * 1 * #{s}"],
          [[[v] * 2] * 2, "!2 * 2 * #{s}"],
          [[[v] * 3] * 2, "!2 * 3 * #{s}"],
          [[[v] * 2] * 3, "!3 * 2 * #{s}"],
          [[[v] * 40] * 3, "!3 * 40 * #{s}"]
        ].each do |vv, ss|
          it "type: #{ss}" do
            t = NDT.new ss
            x = XND.empty ss
            
            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
            expect(x.size).to eq(vv.size)
          end          
        end
      end
    end

    skip "SymbolicDim" do
      DTYPE_EMPTY_TEST_CASES.each do |_, s|
        [
          [ValueError, "N * #{s}"],
          [ValueError, "10 * N * #{s}"],
          [ValueError, "N * 10 * N * #{s}"],
          [ValueError, "X * 10 * N * #{s}"]
        ].each do |err, ss|
          it "raises error for type: #{ss}" do
            t = NDT.new ss
            
            expect {
              XND.empty t
            }.to raise_error(err)            
          end
        end
      end
    end

    context "EllipsisDim" do
      DTYPE_EMPTY_TEST_CASES.each do |_, s|
        [
          [ValueError, "... * #{s}"],
          [ValueError, "Dims... * #{s}"],
          [ValueError, "... * 10 * #{s}"],
          [ValueError, "B... *2 * 3 * ref(#{s})"],
          [ValueError, "A... * 10 * Some(ref(#{s}))"],
          [ValueError, "B... * 2 * 3 * Some(ref(ref(#{s})))"]
        ].each do |err, ss|
          it "raises error for type: #{ss}" do
            t = NDT.new ss
            
            expect {
              XND.empty ss
            }.to raise_error(err) 
          end
        end
      end
    end

    context "Tuple" do
      DTYPE_EMPTY_TEST_CASES.each do |v, s|
        [
          [[v], "(#{s})"],
          [[[v]], "((#{s}))"],
          [[[[v]]], "(((#{s})))"],

          [[[v] * 0], "(0 * #{s})"],
          [[[[v] * 0]], "((0 * #{s}))"],
          [[[v] * 1], "(1 * #{s})"],
          [[[[v] * 1]], "((1 * #{s}))"],
          [[[v] * 3], "(3 * #{s})"],
          [[[[v] * 3]], "((3 * #{s}))"]
        ].each do |vv, ss|
          it "type: #{ss}" do
            t = NDT.new ss
            x = XND.empty ss
          
            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
            expect(x.size).to eq(vv.size)       
          end
        end
      end
    end

    context "Record" do
      DTYPE_EMPTY_TEST_CASES.each do |v, s|
        [
          [{'x' => v}, "{x: #{s}}"],
          [{'x' => {'y' => v}}, "{x: {y: #{s}}}"],

          [{'x' => [v] * 0}, "{x: 0 * #{s}}"],
          [{'x' => {'y' => [v] * 0}}, "{x: {y: 0 * #{s}}}"],
          [{'x' => [v] * 1}, "{x: 1 * #{s}}"],
          [{'x' => [v] * 3}, "{x: 3 * #{s}}"]
        ].each do |vv, ss|
          it "type: #{ss}" do
            t = NDT.new ss
            x = XND.empty ss
                      
            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
            expect(x.size).to eq(vv.size)
          end
        end
      end
    end

    context "Ref" do
      DTYPE_EMPTY_TEST_CASES.each do |v, s|
        [
          [v, "ref(#{s})"],
          [v, "ref(ref(#{s}))"],
          [v, "ref(ref(ref(#{s})))"],

          [[v] * 0, "ref(0 * #{s})"],
          [[v] * 0, "ref(ref(0 * #{s}))"],
          [[v] * 0, "ref(ref(ref(0 * #{s})))"],
          [[v] * 1, "ref(1 * #{s})"],
          [[v] * 1, "ref(ref(1 * #{s}))"],
          [[v] * 1, "ref(ref(ref(1 * #{s})))"],
          [[v] * 3, "ref(3 * #{s})"],
          [[v] * 3, "ref(ref(3 * #{s}))"],
          [[v] * 3, "ref(ref(ref(3 * #{s})))"]
        ].each do |vv, ss|
          it "type: #{ss}" do
            t = NDT.new ss
            x = XND.empty ss
            
            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
            expect_with_exception :size, x, vv
          end
        end
      end
    end

    context "Constr" do
      DTYPE_EMPTY_TEST_CASES.each do |v, s|
        [
          [v, "SomeConstr(#{s})"],
          [v, "Just(Some(#{s}))"],

          [[v] * 0, "Some(0 * #{s})"],
          [[v] * 1, "Some(1 * #{s})"],
          [[v] * 3, "Maybe(3 * #{s})"]
        ].each do |vv, ss|
          it "type: #{ss}" do
            t = NDT.new ss
            x = XND.empty ss
            
            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
            expect_with_exception :size, x, vv
          end
        end
      end
    end

    context "Nominal" do
      c = 0
      DTYPE_EMPTY_TEST_CASES.each do |v, s|
        NDT.typedef "some#{c}", s
        NDT.typedef "just#{c}", "some#{c}"
        
        [
          [v, "some#{c}"],
          [v, "just#{c}"]
        ].each do |vv, ss|
          it "type: #{ss}" do
            t = NDT.new ss
            x = XND.empty ss
            
            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
            expect_with_exception :size, x, vv
          end          
        end

        c += 1
      end
    end

    context "Categorical" do
      # Categorical values are stored as indices into the type's categories.
      # Since empty xnd objects are initialized to zero, the value of an
      # empty categorical entry is always the value of the first category.
      # This is safe, since categorical types must have at least one entry.
      r = {'a' => "", 'b' => 1.2}
      rt = "{a: string, b: categorical(1.2, 10.0, NA)}"

      [
        ["January", "categorical('January')"],
        [[nil], "(categorical(NA, 'January', 'August'))"],
        [[[1.2] * 2] * 10, "10 * 2 * categorical(1.2, 10.0, NA)"],
        [[[100] * 2] * 10, "10 * 2 * categorical(100, 'mixed')"],
        [[[r] * 2] * 10, "10 * 2 * #{rt}"],
        [[[r], [r] * 5, [r] * 3], "var(offsets=[0,3]) * var(offsets=[0,2,7,10]) * #{rt}"]
      ].each do |v, s|

        it "type: #{s}" do
          t = NDT.new s
          x = XND.empty s

          expect(x.type).to eq(t)
          expect(x.value).to eq(v)     
        end
      end
    end

    context "FixedString", v: true do
      it "tests kind of string" do
        expect {
          XND.empty "FixedString"
        }.to raise_error(ValueError)
      end

      [
        ["fixed_string(1)", ""],
        ["fixed_string(3)", "" * 3],
        ["fixed_string(1, 'ascii')", ""],
        ["fixed_string(3, 'utf8')", "" * 3],
        ["fixed_string(3, 'utf16')", "" * 3],
        ["fixed_string(3, 'utf32')", "" * 3],
        ["2 * fixed_string(3, 'utf32')", ["" * 3] * 2],
      ].each do |s, v|
        
        it "type: #{s}" do
          t = NDT.new s
          x = XND.empty s

          expect(x.type).to eq(t)
          expect(x.value).to eq(v)
        end
      end
    end

    context "FixedBytes", v: true do
      # TODO: figure how to deal with byte strings in Ruby.
    end

    context "String", v: true do
      [
        'string',
        '(string)',
        '10 * 2 * string',
        '10 * 2 * (string, string)',
        '10 * 2 * {a: string, b: string}',
        'var(offsets=[0,3]) * var(offsets=[0,2,7,10]) * {a: string, b: string}'
      ].each do |s|

        it "type: #{s}" do
          t = NDT.new s
          x = XND.empty s
          expect(x.type).to eq(t)          
        end
      end

      it "tests for single value" do
        t = NDT.new "string"
        x = XND.empty t

        expect(x.type).to eq(t)
        expect(x.value).to eq('')
      end

      it "tests for multiple values" do
        t = NDT.new "10 * string"
        x = XND.empty t

        expect(x.type).to eq(t)
        0.upto(10) do |i| 
          expect(x[i]).to eq(XND.new(['']))
        end
      end
    end

    context "Bytes", focus: true do
      r = { 'a' => "".b, 'b' => "".b }
      
      [
        [''.b, 'bytes(align=16)'],
        [[''.b], '(bytes(align=32))'],
        [[[''.b] * 2] * 3, '3 * 2 * bytes'],
        [[[[''.b, ''.b]] * 2] * 10, '10 * 2 * (bytes, bytes)'],
        [[[r] * 2] * 10, '10 * 2 * {a: bytes(align=32), b: bytes(align=1)}'],
        [[[r] * 2] * 10, '10 * 2 * {a: bytes(align=1), b: bytes(align=32)}'],
        [[[r] * 2, [r] * 5, [r] * 3], 'var(offsets=[0,3]) * var(offsets=[0,2,7,10]) * {a: bytes(align=32), b: bytes}']
      ].each do |v, s|
        it "type: #{s}" do
          t = NDT.new s
          x = XND.empty t

          expect(x.type).to eq(t)
          expect(x.value).to eq(v)
        end
      end
    end

    context "Char" do
      it "raises NotImplementedError" do
        expect {
          XND.empty "char('utf8')"
        }.to raise_error(NotImplementedError)
      end
    end

    context "SignedKind" do
      it "raises ValueError" do
        expect {
          XND.empty "Signed"
        }.to raise_error(ValueError)
      end
    end

    context "UnsignedKind" do
      it "raises ValueError" do
        expect {
          XND.empty "Unsigned"
        }.to raise_error(ValueError)
      end
    end

    context "FloatKind" do
      it "raises ValueError" do
        expect {
          XND.empty "Float"
        }.to raise_error(ValueError)
      end
    end

    context "ComplexKind" do
      it "raises ValueError" do
        expect {
          XND.empty "Complex"
        }.to raise_error(ValueError)
      end
    end

    context "Primitive" do
      EMPTY_TEST_CASES.each do |value, type_string|
        PRIMITIVE.each do |p|
          ts = type_string % p

          it "type: #{ts}" do
            x = XND.empty ts

            expect(x.value).to eq(value)
            expect(x.type).to eq(NDT.new(ts))
          end
        end
      end
    end

    context "TypeVar" do
      [
        "T",
        "2 * 10 * T",
        "{a: 2 * 10 * T, b: bytes}"
      ].each do |ts|
        it "#{ts} raises ValueError" do
          expect {
            XND.empty ts
          }.to raise_error(ValueError)
        end        
      end
    end
  end

  context ".from_buffer" do
    it "can import data from nmatrix objects" do
      
    end

    it "can import data from narray objects" do
      
    end
  end

  context "#[]" do
    context "FixedDim" do
      it "returns single number slice for 1D array/1 number" do
        xnd = XND.new([1,2,3,4])
        expect(xnd[1]).to eq(XND.new(2))
      end

      it "returns single number slice for 2D array and 2 indices" do
        xnd = XND.new([[1,2,3], [4,5,6]])
        expect(xnd[0,0]).to eq(XND.new(1)) 
      end

      it "returns row for single index in 2D array" do
        x = XND.new [[1,2,3], [4,5,6], [7,8,9]]
        expect(x[1]).to eq(XND.new([4,5,6]))
      end

      it "returns single column in 2D array" do
        x = XND.new [[1,2,3], [4,5,6], [7,8,9]]
        expect(x[0..Float::INFINITY, 0]).to eq(XND.new([1,4,7]))
      end      
    end
  end

  context "#[]=" do
    context "FixedDim" do
      before do
        
      end
    end
  end
  
  context "#strict_equal" do
    context "FixedDim" do
      before do
        @x = XND.new [1,2,3,4]      
      end
      
      it "tests simple arrays" do
        x1 = XND.new [1,2,3,4]

        expect_strict_equal @x, x1
      end

      it "tests different shape and/or data" do
        expect_strict_unequal @x, XND.new([1,2,3,5])
        expect_strict_unequal @x, XND.new([1,2,3,100])
        expect_strict_unequal @x, XND.new([4,2,3,4,5])
      end

      it "tests different shape" do
        expect_strict_unequal @x, XND.new([1,2,3])
        expect_strict_unequal @x, XND.new([[1,2,3,4]])
        expect_strict_unequal @x, XND.new([[1,2], [3,4]])
      end

      it "tests simple multidim array" do
        x = XND.new([[1,2,3], [4,5,6], [7,8,9], [10,11,12]])
        y = XND.new([[1,2,3], [4,5,6], [7,8,9], [10,11,12]])

        expect_strict_equal x, y
      end

      it "tests slices" do
        x = XND.new([[1,2,3], [4,5,6], [7,8,9], [10,11,12]])
        y = XND.new([1,2,3])

        expect_strict_equal x[0], y

        y = XND.new [1,4,7,10]

        expect_strict_equal x[0..Float::INFINITY,0], y
      end

      it "tests corner cases and many dtypes" do
        
      end
    end
  end

  context "#to_a" do
    context "FixedDim" do
      it "returns simple array" do
        x = XND.new [1,2,3,4]

        expect(x.to_a).to eq([1,2,3,4])
      end

      it "returns multi-dim array" do
        x = XND.new [[1,2,3], [4,5,6]]

        expect(x.to_a).to eq([[1,2,3], [4,5,6]])
      end      
    end
  end

  context "#type" do
    it "returns the type of the XND array" do
      x = XND.new [[1,2,3], [4,5,6]], type: NDT.new("2 * 3 * int64")

      expect(x.type).to eq(NDT.new("2 * 3 * int64"))
    end
  end

  context "#to_s" do
    it "returns String representation" do
      
    end
  end

  context "#size" do
    context "FixedDim" do
      it "returns the size of the XND array" do
        x = XND.new [1,2,3,4,5]
        expect(x.size).to eq(5)
      end
    end
  end
end

