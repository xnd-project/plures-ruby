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

## structs in XND

The primary struct that contains data for the XND type is the following:
```
typedef struct XndObject {
  VALUE mblock;              /* owner of the primary type and memory block */
  VALUE type;                /* owner of the current type. lives and dies with this obj. */
  xnd_t xnd;                 /* typed view, does not own anything */
} XndObject;
```
As the comments say, the `mblock` is an object of type `MemoryBlockObject` that
is never revealed to the user. It is shared between multiple instances of XND objects
and contains the primary type (i.e the type of the root object).

The `type` attribute is of type `NDT` and exists only on a per-object basis. It is specific
to the particular instance of `XndObject`. Therefore, whenever making a view, it is important
to store a reference to the `mblock` in the GC guard so that the memory that the view needs
to access for its data needs does not get GC'd in case the root object needs to be GC'd.
