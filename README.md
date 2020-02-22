# simd_util

Utility headers, macros, and functions for non-traditional SIMD use cases (things other than bulk number crunching).

## About This Repository

### Supported Targets
Currently this only targets x86_64 CPUs supporting (at least) `AVX-512F`/`BW`/`DQ`/`VL`/`CD`.
This includes (at present):
  * Sky Lake Xeon and Sky Lake X
  * One (now discontinued) Cannon Lake NUC
  * Ice Lake Client *and* Server (Yay!)

This does *not* include Knights Landing because it's an architectural dead end
and while it'd be _damn cool_ if Intel started putting their AVX-512PF
instructions in "normal" CPUs, the benefit of doing so may be less for your
normal desktop or server parts which have fewer DRAM channels, fewer tiers
of NUMA distance, etc.

### Compiler Notes

For the most part, this code is written with the assumption that you're
using a version of `gcc` >= 7.x and that your binutils is new enough to
supoprt all the AVX-512 variants your CPU supports (primarily this is
important for `objdump`, `gdb`, and `gas` though at that point you might as
well just upgrate the whole binutils suite.

I say `gcc` because that's what I test with, but ultimately so long as the compiler you
intend to use supports the ICC style intrinsics (the `gcc` style ones have
spotty documentation otherwise I'd use them, but they're only documented for
AVX-512 in the internal headers referenced by `immintrin.h` whereas
targeting the Intel intrinsics (while ugly in the source code) provides
decent compatability with `icc` and `clang` but YMMV.

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
  
 
## Possible future inclusions:

  * A source code elaboration utility to support Struct-of-Arrays representations and generate:
    * Accessor macros for Struct-of-Arrays types.
    * Transpose functions for packed Array-of-Structs <--> Struct-of-Arrays
    * Transpose functions for scatter/gather Array-of-Structs <--> Struct-of-Arrays
  * Source code elaboration for some common design patterns:
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
    
