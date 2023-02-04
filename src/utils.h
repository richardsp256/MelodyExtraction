/// @file     utils.h
/// @brief    Declaration/implementation of utility functions
///
/// This should include small, inline functions that may be used in multiple
/// modules (or that need to be reused by the testing infrastructure)

#include <assert.h>
#include <stdlib.h> // aligned_alloc

/// Detects whether the machine is little endian
static inline int IsLittleEndian(){
	// see https://stackoverflow.com/a/12792301/4538758 and	
	// https://en.wikipedia.org/wiki/Endianness#Optimization
	volatile union{
		int32_t i;
		char c[4];
	} converter = { .i = 1 };

	return (converter.c[0] == 1);
}

/// Allocates aligned memory.
///
/// This explicitly checks whether the size is an integer multiple of size
static inline void* AlignedAlloc(size_t alignment, size_t size){
	assert( ((size % alignment) == 0) && (size > 0)); // sanity check
	return aligned_alloc(alignment, size);
}

/// Checks whether a pair of pointers overlap in memory
static inline int HasOverlap(const void * const a, size_t a_length,
                      const void * const b, size_t b_length,
                      size_t elem_size){

	size_t a_start = (size_t) a;
	size_t b_start = (size_t) b;

	if (a_start == b_start){
		return 1;
	} else if (a_start > b_start){
		return (b_start + elem_size * b_length) > a_start;
	} else {
		return (a_start + elem_size * a_length) > b_start;
	}
}
