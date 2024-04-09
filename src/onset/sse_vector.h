// Include this file in calc_correntrograms.c when SSE intrinsics used
// This requires ssse3 (not a typo) instruction support

#include <immintrin.h>
#include <assert.h>

#ifndef SSE_VECTOR_H
#define SSE_VECTOR_H

inline const char* vector_backend_name() { return "SSE Intrinsics"; }

typedef __m128 f32x4;

#define load_f32x4(p) _mm_load_ps(p)
#define store_f32x4(p, a) _mm_store_ps(p, a)
#define broadcast_scalar_f32x4(scalar) _mm_set1_ps(scalar)
#define add_f32x4(a,b) _mm_add_ps(a,b)
#define sub_f32x4(a,b) _mm_sub_ps(a,b)
#define mul_f32x4(a,b) _mm_mul_ps(a,b)

// this requires ssse3 support
static inline __m128 lshift_extract_f32x4(__m128 left, __m128 right,
					  int nlanes)
{
	// For both x86 SSE (see https://stackoverflow.com/a/45110672/4538758)
	// and ARM NEON, the convention is to visualize elements load into
	// vector registers in MSE-first ordering (Most Significan element
	// ordering)
	// - So if you have a 4 element array loaded into a register memory,
	//   the order would be visualized as:  [d, c, b, a]
	// - I find this very counter-intuitive; it's the opposite way that I
	//   visualize arrays. It's also the reverse of how C-arrays are
	//   initialized
	// - It's apparently the way that all vector intrinsics are labelled.
	//
	// Because we are only using a subset of vector intrinsics, we are
	// ignoring the convention and using LSE-first ordering

	// Given a left vector, [a b c d] and right vector, [e f g h]
	//   1. concatenates both vectors [a b c d e f g h]
	//   2. shift the vectors N lanes to the right
	//      N = 1:   [b c d e f g h 0]
	//      N = 2:   [c d e f g h 0 0]
	//      N = 3:   [d e f g h 0 0 0]
	//   3. extracts and returns the rightmost 4 elements
	//      N = 1:   [b c d e]
	//      N = 2:   [c d e f]
	//      N = 3:   [d e f g]

	assert(nlanes < 4 && nlanes >= 0);

	__m128i int_l  = _mm_castps_si128(left); // cast left to int_l
	__m128i int_r = _mm_castps_si128(right); // cast right to int_r

	// clang requires that the integer passed to _mm_alignr_epi8 is known
	// compile time
	__m128 result;
	switch(nlanes){
	case 0: return left;
	case 1: result = _mm_castsi128_ps(_mm_alignr_epi8(int_r,int_l,4));
		break;
	case 2: result = _mm_castsi128_ps(_mm_alignr_epi8(int_r,int_l,8));
		break;
	case 3: result = _mm_castsi128_ps(_mm_alignr_epi8(int_r,int_l,12));
		break;
	default: abort();
	}
	return result;
}






#endif /* SSE_VECTOR_H */
