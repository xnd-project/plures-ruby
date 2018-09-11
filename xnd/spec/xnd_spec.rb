require 'spec_helper'

describe XND do
  context ".new" do
    context "Type Inference" do
      context "Tuple" do
        d = {'a' => XND::T.new(2.0, "bytes".b), 'b' => XND::T.new("str", Float::INFINITY) }
        typeof_d = "{a: (float64, bytes), b: (string, float64)}"

        [
          [XND::T.new(), "()"],
          [XND::T.new(XND::T.new()), "(())"],
          [XND::T.new(XND::T.new(), XND::T.new()), "((), ())"],
          [XND::T.new(XND::T.new(XND::T.new()), XND::T.new()), "((()), ())"],
          [XND::T.new(XND::T.new(XND::T.new()), XND::T.new(XND::T.new(), XND::T.new())),
           "((()), ((), ()))"],
          [XND::T.new(1, 2, 3), "(int64, int64, int64)"],
          [XND::T.new(1.0, 2, "str"), "(float64, int64, string)"],
          [XND::T.new(1.0, 2, XND::T.new("str", "bytes".b, d)),
           "(float64, int64, (string, bytes, #{typeof_d}))"]
        ].each do |v, t|
          it "type: #{t}" do
            x = XND.new v

            expect(x.type).to eq(NDT.new(t))
            expect(x.value).to eq(XND::TypeInference.convert_xnd_t_to_ruby_array(v))
          end
        end
      end

      context "Record" do
        d = {'a' => XND::T.new(2.0, "bytes".b), 'b' => XND::T.new("str", Float::INFINITY) }
        typeof_d = "{a: (float64, bytes), b: (string, float64)}"

        [
          [{}, "{}"],
          [{'x' => {}}, "{x: {}}"],
          [{'x' => {}, 'y' => {}}, "{x: {}, y: {}}"],
          [{'x' => {'y' => {}}, 'z' => {}}, "{x: {y: {}}, z: {}}"],
          [{'x' => {'y' => {}}, 'z' => {'a' => {}, 'b' => {}}}, "{x: {y: {}}, z: {a: {}, b: {}}}"],
          [d, typeof_d]
        ].each do |v, t|
          it "type: #{t}" do
            x = XND.new v

            expect(x.type).to eq(NDT.new(t))
            expect(x.value).to eq(v)            
          end
        end
      end

      context "Float64" do
        d = {'a' => 2.221e100, 'b' => Float::INFINITY}
        typeof_d = "{a: float64, b: float64}"

        [
          # 'float64' is the default dtype if there is no data at all.
          [[], "0 * float64"],
          [[[]], "1 * 0 * float64"],
          [[[], []], "2 * 0 * float64"],
          [[[[]], [[]]], "2 * 1 * 0 * float64"],
          [[[[]], [[], []]],
           "var(offsets=[0, 2]) * var(offsets=[0, 1, 3]) * var(offsets=[0, 0, 0, 0]) * float64"],

          [[0.0], "1 * float64"],
          [[0.0, 1.2], "2 * float64"],
          [[[0.0], [1.2]], "2 * 1 * float64"],

          [d, typeof_d],
          [[d] * 2, "2 * %s" % typeof_d],
          [[[d] * 2] * 10, "10 * 2 * #{typeof_d}"]
        ].each do |v, t|
          it "type: #{t}" do
            x = XND.new v

            expect(x.type).to eq(NDT.new(t))
            expect(x.value).to eq(v)            
          end
        end
      end

      context "Complex128" do
        d = {'a' => 3.123+10i, 'b' => Complex(Float::INFINITY, Float::INFINITY)}
        typeof_d = "{a: complex128, b: complex128}"

        [
          [[1+3e300i], "1 * complex128"],
          [[-2.2-5i, 1.2-10i], "2 * complex128"],
          [[-2.2-5i, 1.2-10i, nil], "3 * ?complex128"],
          [[[-1+3i], [-3+5i]], "2 * 1 * complex128"],

          [d, typeof_d],
          [[d] * 2, "2 * #{typeof_d}"],
          [[[d] * 2] * 10, "10 * 2 * #{typeof_d}"]
        ].each do |v, t|
          it "type: #{t}" do
            x = XND.new v

            expect(x.type).to eq(NDT.new(t))
            expect(x.value).to eq(v)            
          end
        end
      end

      context "Int64" do
        t = XND::T.new(1, -2, -3)
        typeof_t = "(int64, int64, int64)"

        [
          [[0], "1 * int64"],
          [[0, 1], "2 * int64"],
          [[[0], [1]], "2 * 1 * int64"],

          [t, typeof_t],
          [[t] * 2, "2 * #{typeof_t}"],
          [[[t] * 2] * 10, "10 * 2 * #{typeof_t}"]
        ].each do |v, t|
          it "type: #{t}" do  
            x = XND.new v

            expect(x.type).to eq(NDT.new(t))
            expect(x.value).to eq(XND::TypeInference.convert_xnd_t_to_ruby_array(v))
          end
        end
      end

      context "String" do
        t = XND::T.new("supererogatory", "exiguous")
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
            expect(x.value).to eq(XND::TypeInference.convert_xnd_t_to_ruby_array(v))
          end
        end
      end

      context "Bytes" do
        t = XND::T.new("lagrange".b, "points".b)
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
            expect(x.value).to eq(XND::TypeInference.convert_xnd_t_to_ruby_array(v))
          end
        end
      end

      context "Optional" do
        [
          [nil, "?float64"],
          [[nil], "1 * ?float64"],
          [[nil, nil], "2 * ?float64"],
          [[nil, 10], "2 * ?int64"],
          [[nil, 'abc'.b], "2 * ?bytes"],
          [[nil, 'abc'], "2 * ?string"]
        ].each do |v, t|
          it "type: #{t}" do
            x = XND.new v

            expect(x.type).to eq(NDT.new(t))
            expect(x.value).to eq(v)
          end
        end

        [
          [nil, []],
          [[], nil],
          [nil, [10]],
          [[nil, [0, 1]], [[2, 3]]]
        ].each do |v|
          it "not implemented for value: #{v}" do 
            expect {
              XND.new v
            }.to raise_error(NotImplementedError)           
          end
        end
      end # context Optional
    end # context TypeInference
    
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

      it "raises ValueError for wrong input type in int64 array" do
        t = NDT.new "2 * 3 * int64"
        expect {
          XND.new([[1,2,"peep!"], [2,3,4]], type: t)
        }.to raise_error(TypeError)      
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

    context "String" do
      it "creates new String array" do
        t = '2 * {a: complex128, b: string}'
        x = XND.new([{'a' => 2+3i, 'b' => "thisguy"},
                    {'a' => 1+4i, 'b' => "thatguy"}], type: t)

        expect(x[0]['b']).to eq("thisguy")
        expect(x[1]['b']).to eq("thatguy")
      end
    end # context String

    context "Bool" do
      it "from bool" do
        x = XND.new true, type: "bool"
        expect(x.value).to eq(true)

        x = XND.new false, type: "bool"
        expect(x.value).to eq(false)
      end

      it "from int" do
        x = XND.new 1, type: "bool"
        expect(x.value).to eq(true)

        x = XND.new 0, type: "bool"
        expect(x.value).to eq(false)
      end

      it "from object" do
        x = XND.new [1,2,3], type: "bool"
        expect(x.value).to eq(true)

        x = XND.new nil, type: "?bool"
        expect(x.value).to eq(nil)

        expect {
          XND.new nil, type: "bool"
        }.to raise_error(TypeError)
      end

      skip "tests broken input - how can this be done in Ruby?" do
        
      end
    end # context Bool

    context "Signed" do
      [8, 16, 32, 64].each do |n|
        it "tests bounds for n=#{n}" do
          t = "int#{n}"

          v = -2**(n-1)
          x = XND.new(v, type: t)
          expect(x.value).to eq(v)
          expect { XND.new v-1, type: t }.to raise_error { |error|
            error.should be_a(ValueError || OverflowError)
          }

          v = 2**(n-1) - 1
          x = XND.new(v, type: t)
          expect(x.value, v)
          expect { XND.new v+1, type: t }.to raise_error { |error| 
            error.should be_a(ValueError || OverflowError)
          }
        end
      end
    end # context Signed

    context "Unsigned" do
      [8, 16, 32, 64].each do |n|
        it "tests bounds n=#{n}" do
          t = "uint#{n}"

          v = 0
          x = XND.new v, type: t
          expect(x.value).to eq(v) 
          expect { XND.new v-1, type: t }.to raise_error { |error|
            error.should be_a(ValueError || OverflowError)
          }

          v = 2**n - 1
          x = XND.new v, type: t
          expect(x.value).to eq(v)
          expect { XND.new v+1, type: t }.to raise_error { |error|
            error.should be_a(ValueError || OverflowError)
          }
        end
      end
    end # context Unsigned

    context "Float32" do
      it "tests bounds" do
        denorm_min = Float.from_hex "0x1p-149"
        lowest = Float.from_hex "-0x1.fffffep+127"
        max = Float.from_hex "0x1.fffffep+127"
        inf = Float.from_hex "0x1.ffffffp+127"

        x = XND.new denorm_min, type: "float32"
        expect(x.value).to eq(denorm_min)

        x = XND.new lowest, type: "float32"
        expect(x.value).to eq(lowest)

        x = XND.new max, type: "float32"
        expect(x.value).to eq(max)

        expect { XND.new(inf, type: "float32") }.to raise_error(OverflowError)
        expect { XND.new(-inf, type: "float32") }.to raise_error(OverflowError)
      end

      it "tests special values" do
        x = XND.new Float::INFINITY, type: "float32"
        expect(x.value.infinite?).to eq(true)

        x = XND.new Float::NaN, type: "float32"
        expect(x.value.nan?).to eq(true)
      end
    end # context Float32

    context "Float64" do
      it "tests bounds" do
        denorm_min = Float.from_hex "0x0.0000000000001p-1022"
        lowest = Float.from_hex "-0x1.fffffffffffffp+1023"
        max = Float.from_hex "0x1.fffffffffffffp+1023"

        x = XND.new denorm_min, type: "float64"
        expect(x.value).to eq(denorm_min)

        x = XND.new lowest, type: "float64"
        expect(x.value).to eq(lowest)

        x = XND.new max, type: "float64"
        expect(x.value).to eq(max)
      end

      it "tests special values" do
        x = XND.new Float::INFINITY, type: "float64"
        expect(x.value.infinite?).to eq(true)

        x = XND.new Float::NaN, type: "float64"
        expect(x.value.infinite?).to eq(true)
      end
    end # context Float64

    context "Complex64" do
      it "tests bounds" do
        denorm_min = Float.from_hex "0x1p-149"
        lowest = Float.from_hex "-0x1.fffffep+127"
        max = Float.from_hex "0x1.fffffep+127"
        inf = Float.from_hex "0x1.ffffffp+127"

        v = Complex(denorm_min, denorm_min)
        x = XND.new v, type: "complex64"
        expect(x.value).to eq(v)

        v = Complex(lowest, lowest)
        x = XND.new v, type: "complex64"
        expect(x.value).to eq(v)

        v = Complex(max, max)
        x = XND.new v, type: "complex64"
        expect(x.value).to eq(v)

        v = Complex(inf, inf)
        expect { XND.new v, type: "complex64" }.to raise_error(OverflowError)

        v = Complex(-inf, -inf)
        expect { XND.new v, type: "complex64" }.to raise_error(OverflowError)
      end

      it "tests special values" do
        x = XND.new Complex(Float::INFINITY, 0), type: "complex64"
        expect(x.value.real.infinite?).to eq(true)
        expect(x.value.imag).to eq(0.0)

        x = XND.new Complex(Float::NaN, 0), type: "complex64"
        expect(x.value.real.infinite?).to eq(true)
        expect(x.value.imag).to eq(0.0)
      end
    end # context Complex64

    context "Complex128" do
      it "tests bounds" do
        denorm_min = Float.from_hex("0x0.0000000000001p-1022")
        lowest = Float.from_hex("-0x1.fffffffffffffp+1023")
        max = Float.from_hex("0x1.fffffffffffffp+1023")

        v = Complex(denorm_min, denorm_min)
        x = XND.new v, type: "complex128"
        expect(x.value).to eq(v)

        v = Complex(lowest, lowest)
        x = XND.new v, type: "complex128"
        expect(x.value).to eq(v)

        v = Complex(max, max)
        x = XND.new v, type: "complex128"
        expect(x.value).to eq(v)
      end

      it "tests special values" do
        x = XND.new Complex(Float::INFINITY), type: "complex128"

        expect(x.value.real.infinite?).to eq(true)
        expect(x.value.imag).to eq(0.0)

        x = XND.new Complex(Float::NaN), type: "complex128"

        expect(x.value.real.infinite?).to eq(true)
        expect(x.value.imag).to eq(0.0)
      end
    end # context Complex128
  end # context .new

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

      it "returns empty view" do
        inner = [[0+0i] * 5] * 4
        x = XND.empty "2 * 3 * ref(4 * 5 * complex128)"

        y = x[1][2]
        expect(y.is_a?(XND)).to eq(true)
        expect(y.value).to eq(inner)

        y = x[1, 2]
        expect(y.is_a?(XND)).to eq(true)
        expect(y.value).to eq(inner)
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

    context "SymbolicDim" do
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
          it "type: #{ss}", vag: true do
            t = NDT.new ss
            x = XND.empty ss
            
            expect(x.type).to eq(t)
            expect(x.value).to eq(vv)
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
            if vv == 0
              expect {
                x.size
              }.to raise_error(NoMethodError)
            end
          end
        end
      end

      it "returns empty view" do
        # If a constr is a dtype but contains an array itself, indexing should
        # return a view and not a Python value.
        inner = [[""] * 5] * 4
        x = XND.empty("2 * 3 * InnerArray(4 * 5 * string)")

        y = x[1][2]
        expect(y.is_a?(XND)).to eq(true)
        expect(y.value).to eq(inner)

        y = x[1, 2]
        expect(y.is_a?(XND)).to eq(true)
        expect(y.value).to eq(inner)
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
            if vv == 0
              expect {
                x.size
              }.to raise_error(NoMethodError)
            end
          end          
        end

        c += 1
      end

      it "returns empty view" do
        # If a typedef is a dtype but contains an array itself, indexing should
        # return a view and not a Python value.
        NDT.typedef("inner_array", "4 * 5 * string")
        inner = [[""] * 5] * 4
        x = XND.empty("2 * 3 * inner_array")

        y = x[1][2]
        expect(y.is_a?(XND)).to eq(true)
        expect(y.value).to eq(inner)

        y = x[1, 2]
        expect(y.is_a?(XND)).to eq(true)
        expect(y.value).to eq(inner)
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
        [[[r] * 2, [r] * 5, [r] * 3], "var(offsets=[0,3]) * var(offsets=[0,2,7,10]) * #{rt}"]
      ].each do |v, s|

        it "type: #{s}" do
          t = NDT.new s
          x = XND.empty s

          expect(x.type).to eq(t)
          expect(x.value).to eq(v)     
        end
      end
    end

    context "FixedString" do
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

    context "FixedBytes" do
      r = {'a' => "\x00".b * 3, 'b' => "\x00".b * 10}

      [
        ["\x00".b, 'fixed_bytes(size=1)'],
        ["\x00".b * 100, 'fixed_bytes(size=100)'],
        ["\x00".b * 4, 'fixed_bytes(size=4, align=2)'],
        ["\x00".b * 128, 'fixed_bytes(size=128, align=16)'],
        [r, '{a: fixed_bytes(size=3), b: fixed_bytes(size=10)}'],
        [[[r] * 3] * 2, '2 * 3 * {a: fixed_bytes(size=3), b: fixed_bytes(size=10)}']
      ].each do |v, s|
        it "type: #{s}" do
          t = NDT.new s
          x = XND.empty s

          expect(x.type).to eq(t)
          expect(x.value).to eq(v)          
        end
      end
    end

    context "String" do
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

    context "Bytes" do
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
      it "raises ValueError" do
        expect {
          XND.empty "char('utf8')"
        }.to raise_error(ValueError)
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

    context "FixedBytesKind" do
      it "raises ValueError" do
        expect {
          XND.empty "FixedBytes"
        }.to raise_error(ValueError)
      end
    end

    context "Primitive" do
      empty_test_cases.each do |value, type_string|
        PRIMITIVE.each do |p|
          ts = type_string % p

          it "type: #{ts}" do
            x = XND.empty ts

            expect(x.value).to eq(value)
            expect(x.type).to eq(NDT.new(ts))
          end
        end
      end

      empty_test_cases(false).each do |value, type_string|
        BOOL_PRIMITIVE.each do |p|
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

      it "returns the entire array" do
        x = XND.new [[1,2,3], [4,5,6], [7,8,9]]
        expect(x[0..Float::INFINITY]).to eq(x)
      end

      [
        [
          [
           [11.12-2.3i, -1222+20e8i],
           [Complex(Float::INFINITY, Float::INFINITY), -0.00002i],
           [0.201+1i, -1+1e301i]
          ], "3 * 2 * complex128"],
        [
          [
            [11.12-2.3i, nil],
            [Complex(Float::INFINITY, Float::INFINITY), nil],
            [0.201+1i, -1+1e301i]
          ], "3 * 2 * ?complex128"]
      ].each do |v, s|
        context "type: #{s}" do
          before do
            @arr = v
            @t = NDT.new s
            @x = XND.new v, type: @t
          end

          it "check values" do
            expect(@x.to_a).to eq(@arr.to_a)
          end

          0.upto(2) do |i|
            it "value: i= #{i}" do
              expect(@x[i].to_a).to eq(@arr[i])
            end
          end

          3.times do |i|
            2.times do |k|
              it "value: i=#{i}. k=#{k}" do
                expect(@x[i][k].value).to eq(@arr[i][k])
                expect(@x[i, k].value).to eq(@arr[i][k])
              end
            end
          end

          it "tests full slices" do
            expect(@x[INF].value).to eq(@arr)
          end

          ((-3...4).to_a + [Float::INFINITY]).each do |start|
            ((-3...4).to_a + [Float::INFINITY]).each do |stop|
              [true, false].each do |exclude_end|
                # FIXME: add step count when ruby supports it.
                arr_s = get_inf_or_normal_range start, stop, exclude_end

                it "Range[#{start}, #{stop}#{exclude_end ? ')' : ']'}" do
                  r = Range.new(start, stop, exclude_end)
                  expect(@x[r].value).to eq(@arr[arr_s])
                end
              end
            end
          end

          it "tests single rows" do
            expect(@x[INF, 0].value).to eq(@arr.transpose[0])
            expect(@x[INF, 1].value).to eq(@arr.transpose[1])
          end
        end
      end
    end

    context "Fortran" do
      [
        [[[11.12-2.3i, -1222+20e8i],
          [Complex(Float::INFINITY, Float::INFINITY), -0.00002i],
          [0.201+1i, -1+1e301i]], "!3 * 2 * complex128"],
        [[[11.12-2.3i, nil],
          [Complex(Float::INFINITY, Float::INFINITY), nil],
          [0.201+1i, -1+1e301i]], "!3 * 2 * ?complex128"]
      ].each do |v, s|
        context "type: #{s}" do
          before do
            @arr = v
            @t = NDT.new s
            @x = XND.new v, type: @t
          end

          (0).upto(2) do |i|
            it "check row i= #{i}" do
              expect(@x[i].value).to eq(@arr[i])              
            end
          end

          (0).upto(2) do |i|
            (0).upto(1) do |k|
              it "check elements i=#{i} k=#{k}" do
                expect(@x[i][k].value).to eq(@arr[i][k])
                expect(@x[i, k].value).to eq(@arr[i][k])
              end
            end
          end

          it "checks full slice" do
            expect(@x[INF].to_a).to eq(@arr)
          end

          ((-3..-3).to_a + [Float::INFINITY]).each do |start|
            ((-3..-3).to_a + [Float::INFINITY]).each do |stop|
              [true, false].each do |exclude_end|
                # FIXME: add step count loop post Ruby 2.6
                arr_s = get_inf_or_normal_range start, stop, exclude_end

                it "Range[#{start}, #{stop}#{exclude_end ? ')' : ']'}" do
                  r = Range.new start, stop, exclude_end
                  expect(@x[r].value).to eq(@arr[arr_s])
                end
              end
            end
          end

          it "checks column slices" do
            expect(@x[INF, 0].value).to eq(@arr.transpose[0])
            expect(@x[INF, 1].value).to eq(@arr.transpose[1])
          end
        end
      end
    end

    context "Ref" do
      before do
        # If a ref is a dtype but contains an array itself, indexing through
        # the ref should work transparently.
        @inner = [['a', 'b', 'c', 'd', 'e'],
                 ['f', 'g', 'h', 'i', 'j'],
                 ['k', 'l', 'm', 'n', 'o'],
                 ['p', 'q', 'r', 's', 't']]
        @v = [[@inner] * 3] * 2
        @x = XND.new(@v, type: "2 * 3 * ref(4 * 5 * string)")
      end

      (0).upto(1) do |i|
        (0).upto(2) do |j|
          (0).upto(3) do |k|
            (0).upto(4) do |l|
              it "index: i=#{i} j=#{j} k=#{k} l=#{l}" do
                expect(@x[i][j][k][l]).to eq(@inner[k][l])
                expect(@x[i, j, k, l]).to eq(@inner[k][l])                
              end
            end
          end
        end
      end
    end

    context "Constr" do
      before do
        # If a constr is a dtype but contains an array itself, indexing through
        # the constructor should work transparently.
        @inner = [['a', 'b', 'c', 'd', 'e'],
                 ['f', 'g', 'h', 'i', 'j'],
                 ['k', 'l', 'm', 'n', 'o'],
                 ['p', 'q', 'r', 's', 't']]
        @v = [[@inner] * 3] * 2
        @x = XND.new(@v, type: "2 * 3 * InnerArray(4 * 5 * string)")
      end

      (0).upto(1) do |i|
        (0).upto(2) do |j|
          (0).upto(3) do |k|
            (0).upto(4) do |l|
              it "slice: i=#{i} j=#{j} k=#{k} l=#{l}" do
                expect(@x[i][j][k][l]).to eq(@inner[k][l])
                expect(@x[i, j, k, l]).to eq(@inner[k][l])
              end
            end
          end
        end
      end
    end # context Constr

    context "Nominal" do
      before do
        # If a typedef is a dtype but contains an array itself, indexing through
        # the constructor should work transparently.
        NDT.typedef("inner", "4 * 5 * string")
        @inner = [['a', 'b', 'c', 'd', 'e'],
                 ['f', 'g', 'h', 'i', 'j'],
                 ['k', 'l', 'm', 'n', 'o'],
                 ['p', 'q', 'r', 's', 't']]
        @v = 2 * [3 * [@inner]]
        @x = XND.new(v, type: "2 * 3 * inner")        
      end


      (0).upto(1) do |i|
        (0).upto(2) do |j|
          (0).upto(3) do |k|
            (0).upto(4) do |l|
              it "slice: i=#{i} j=#{j} k=#{k} l=#{l}" do
                expect(@x[i][j][k][l]).to eq(@inner[k][l])
                expect(@x[i, j, k, l]).to eq(@inner[k][l])
              end
            end
          end
        end
      end      
    end # context Nominal
  end # context #[]

  context "#[]=" do
    context "FixedDim" do

    end

    context "Fortran" do
      before do
        @x = XND.empty "!2 * 4 * float64"
        @v = [[0.0, 1.0, 2.0, 3.0], [4.0, 5.0, 6.0, 7.0]]
      end
      
      it "full slice" do
        @x[INF] = @v

        expect(@x.value).to eq(@v)
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
    end # context FixedDim

    context "Float32" do
      
    end
  end # Context #strict_equal

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
  
    context "Bool" do
      it "raises error" do
        x = XND.new true, type: "bool"
        expect {
          x.size
        }.to raise_error(NoMethodError)
      end
    end

    context "Signed" do
      it "raises error" do
        x = XND.new 10, type: "int16"
        expect { x.size }.to raise_error(NoMethodError)
      end
    end

    context "Unsigned" do
      it "raises error" do
        x = XND.new 10, type: "uint64"
        expect { x.size }.to raise_error(NoMethodError)
      end
    end
  end

  context "#each" do
    context "FixedDim" do
      it "iterates over all elements" do
        x = XND.new [1,2,3,4,5], type: "5 * int64"
        sum = 0
        x.each do |a|
          sum += x
        end

        expect(sum).to eq(15)
      end
    end
  end
end

