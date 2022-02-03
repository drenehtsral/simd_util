# simd_util

Utility headers, macros, and functions for non-traditional SIMD use cases (things other than bulk number crunching).

## About This Repository

### Supported Targets
Currently this only targets x86_64 CPUs supporting (at least) `AVX-512F`/`BW`/`DQ`/`VL`/`CD`. This includes (at present):

  * Sky Lake Xeon and Sky Lake X
  * One (now discontinued) Cannon Lake NUC
  * Ice Lake Client *and* Server (Yay!)

This does **not** include Knights Landing because it's an architectural dead end and while it'd be _damn cool_ if Intel started putting their AVX-512PF instructions in "normal" CPUs, the benefit of doing so may be less for your normal desktop or even server parts which have fewer DRAM channels, fewer tiers of NUMA distance, etc.

The AVX-512PF extension is only useful when you're pulling results together from possibly many disparate DRAM channels into a very local cache, which is something that makes more sense in the context of the Knights Landing architecture (also, the Knights Landing cores don't do the aggressive and deep OOO + Speculative execution that the desktop and server parts do so it's more important to stick explicit preferches to avoid stalls on gathers).

### Compiler Notes

For the most part, this code is written with the assumption that you're using a version of `gcc` >= 9.x and that your binutils version is new enough to supoprt all the AVX-512 variants your CPU supports (primarily this is important for `objdump`, `gdb`, and `gas` though at that point you might as well just upgrate the whole binutils suite.

I say `gcc` because that's what I test with, but ultimately so long as the compiler you intend to use supports the ICC style intrinsics (the `gcc` style ones have spotty documentation otherwise I'd use them, but they're only documented for AVX-512 in the internal headers referenced by `immintrin.h` whereas targeting the Intel intrinsics (while ugly in the source code) provides decent compatability with `icc` and `clang` but YMMV.

### What's Inside

This utility repo contains mostly C header files to provide the following: 

 * Type definitions.
 * Macros for trivial operations.
 * Inline functions for non-trivial operations.
 
In addition, there will be some C files to provide a few additional things not easily provided in header form:

  * Unit/regression tests for complex operations.
  * Non-SIMD reference implementations where/when it makes sense to include them.
  * An intentionally opaque/unconstrained selection of `consume_data()` functions to prevent the optimizer from discarding the entire calculation for micro-benchmarks.
  * SIMD type pretty print and visualization.
  * Micro-Benchmarks for operations where they prove useful (these are _always_ to be taken with an enormous grain of salt).
 
### Some Idioms/Patterns to Be Aware Of

This code base uses some patterns to keep code that uses these utility macros consistent and readable.  This section contains notes on some of them to make them a bit more approachable.

#### SIMD Type Names

| Type and Width       | Underscore | Lane Count (XMM, YMM, ZMM) |
|----------------------|------------|----------------------------|
| `u8`, `i8`           |    `_`     | 16, 32, 64                 |
| `u16`, `i16`         |    `_`     | 8, 16, 32                  |
| `u32`, `i32`, `f32`  |    `_`     | 4, 8, 16                   |
| `u64`, `i64`, `f64`  |    `_`     | 2, 4, 8                    |

e.g. a vector containing eight lanes each containing a 32-bit unsigned integer would be expressed as `u32_8` while a vector of 64 signed characters would be `i8_64`.

#### Monster Multipronged Macros

In some files (e.g. [mask_util.h](include/mask_util.h)) you'll see giant macros which combine the use of a couple of tricks to allow for macros that represent the same conceptual operation for any SIMD type you supply:

 * `typeof()` based per-instantiation field types in unions.
 * Compile-time-evaluated built-in functions (like `sizeof()` or `__builtin_types_compatible_p()`) in chained `if`-`else` blocks.
 * Compile-time dead-code elimination (trim unreachable blocks due to compile-time constant expression given to `if`).

A good example of this is `VEC_TO_MASK()` which takes a parameter of **any** of the above SIMD types and returns a mask register value (which the compiler will tend to keep in a mask register if you use it locally as a mask, or store it in a general-purpose integer register otherwise).  For example:
```
    const u32_8 foo = {~0,  0,  0,  0,  0,  0, ~0, ~0};
    printf("0x%X\n", VEC_TO_MASK(foo));
```
will output `0xC1` since the MSBs of lanes 0, 6, and 7 are set.

The only time you (as the user of these utilities) really needs to know or care how these monster macros work is when you pass an expresison to one that itself contains a syntax error because the resulting errors will look a little odd, but modern versions of gcc are actually quite good about specifying the file and line at which the macro expansion that went sideways got invoked from.

## Possible future inclusions:

  * A source code elaboration utility to support Struct-of-Arrays representations and generate:
    * Accessor macros for Struct-of-Arrays types.
    * Transpose functions for packed Array-of-Structs <--> Struct-of-Arrays
    * Transpose functions for scatter/gather Array-of-Structs <--> Struct-of-Arrays
  * Source code elaboration tools for some common design patterns:
    * Pipelined parallel table lookup (amortize L3 or DRAM latency)
    * Parallel state machine update.
    * Binning and batch dispatch with:
      * Latency aware dispatch heuristics.
      * Conflict/Hazard aware batch boundaries.
 
   
## References and Inspirations:
  * http://www.branchfree.org
  * http://www.cs.columbia.edu/~kar/pubsk/simd.pdf
  * https://software.intel.com/sites/landingpage/IntrinsicsGuide/
  * https://software.intel.com/en-us/articles/intel-software-development-emulator
  * https://www.agner.org/optimize/
    
