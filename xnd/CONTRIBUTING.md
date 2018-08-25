# Developer notes

## Using C APIs from other Ruby libraries

The xnd Ruby wrapper uses type definitions, macros and functions from the
libndtypes Ruby wrapper. For this purpose, it is important to make sure that
xnd can find the correct headers and shared object files during compile and
linking time.

This requires some modifications in both the ndtypes and xnd repos. ndtypes
must ensure that the relevant header files and shared object are exposed to
other Ruby gems.

## C API etiqquette

Its important to know what to expose to other Ruby programs via the C API.

const ndt_t * (const PyObject *)

CONST_NDT_INDEX = 2
