# simd_util
Utility headers, macros, and functions for non-traditional SIMD use cases (things other than bulk number crunching).

This utility repo contains mostly C header files to provide the following: 
 * Type definitions.
 * Macros for trivial operations.
 * Inline functions for non-trivial operations.
 
 In addition, there will be some C files to provide a few additional things not easily provided in header form:
  * Unit/regression tests for complex operations.
  * Non-SIMD reference implementations where/when it makes sense to include them.
  * An intentionally opaque/unconstrained selection of `consume_data()` functions to prevent the optimizer from discarding the entire calculation for micro-benchmarks.
  * SIMD type pretty printer functions.
 
 Possible future inclusions:
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
   
   References and Inspirations:
    * http://www.branchfree.org
    * http://www.cs.columbia.edu/~kar/pubsk/simd.pdf
    * https://software.intel.com/sites/landingpage/IntrinsicsGuide/
    * https://software.intel.com/en-us/articles/intel-software-development-emulator
    
