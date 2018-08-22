# plures-ruby

Ruby wrapper for the plures project

Contains folders xnd/, ndtypes/ and gumath/ with their respective wrappers.

The wrappers are written for the following commits of the libraries. It will stay frozen
atleast for the time being.

* xnd - https://github.com/plures/xnd/commit/d85217246eeef8a176210a5dcfdc9455d8403919
* ndtypes - https://github.com/plures/ndtypes/commit/bc304fc21edc033b9e867fe31060f50ecb370e3e
* gumath - https://github.com/plures/gumath/commit/0c269b20f96a51bb51b270320ad6084f2b96b83f

For now I am simply copying pre-built binaries into the repo and using those for
building the wrapper. However I will write a proper installation script for building the gems
from source later.

# Installation

This is a temporary install. The libraries will be segregated and packaged when ready
for deployment.

1. Clone the xnd-all repo : https://github.com/plures/xnd-all
2. Install and build the binaries for your machine.
3. Copy paste the build/ folder and its contents to this directory.
4. `cd` into the library you want to build and follow instructions in that README.

