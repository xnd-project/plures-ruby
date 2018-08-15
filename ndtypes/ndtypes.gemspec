# coding: utf-8
$:.unshift File.expand_path("../lib", __FILE__)

require 'ndtypes/version.rb'

NDTypes::DESCRIPTION = <<MSG
XND is a library for typed data arrays in Ruby. It is a wrapper over the libndtypes C library.
MSG

Gem::Specification.new do |spec|
  spec.name          = 'ndtypes'
  spec.version       = NDTypes::VERSION
  spec.authors       = ['Sameer Deshmukh']
  spec.email         = ['sameer.deshmukh93@gmail.com']
  spec.summary       = %q{Ruby wrapper over libndtypes. A library for typing memory blocks.}
  spec.description   = NDTypes::DESCRIPTION
  spec.homepage      = "https://github.com/Quansight/plures-ruby"
  #spec.license       = 'BSD-2'

  spec.files         = `git ls-files -z`.split("\x0")
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.add_development_dependency 'rspec', '~> 3.8'
  spec.add_development_dependency 'rake-compiler'
end
