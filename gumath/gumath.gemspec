# coding: utf-8
$:.unshift File.expand_path("../lib", __FILE__)

require 'gumath/version.rb'

Gumath::DESCRIPTION = <<MSG
Gumath is library for function dispatch to XND containers.
MSG

Gem::Specification.new do |spec|
  spec.name          = 'gumath'
  spec.version       = Gumath::VERSION
  spec.authors       = ['Sameer Deshmukh']
  spec.email         = ['sameer.deshmukh93@gmail.com']
  spec.summary       = %q{Ruby wrapper over libgumath. A library for dispatching math kernels.}
  spec.description   = Gumath::DESCRIPTION
  spec.homepage      = "https://github.com/Quansight/plures-ruby"
  spec.license       = 'BSD-3 Clause'

  spec.files         = `git ls-files -z`.split("\x0")
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.add_development_dependency 'minitest', '~> 5.11'
  spec.add_development_dependency 'rake-compiler'
  spec.add_development_dependency 'pry'
  spec.add_development_dependency 'pry-byebug'
end
