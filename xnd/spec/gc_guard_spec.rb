require 'spec_helper'

describe RubyXND::GCGuard do
  it "stores objects as key and corresponding mblock object as value" do
    xnd = XND.new([[1,2,3]])

    gc_table = RubyXND::GCGuard.instance_variable_get(:@__gc_guard_table)
    expect(gc_table.keys.size).to eq(1)
  end
end
