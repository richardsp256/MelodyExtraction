/// @file     calcSummedLagCorrentrograms.c
/// @brief    Implementation of calcSummedLagCorrentrograms

#include <stdalign.h> // alignas keyword
#include <stdint.h>   // int32_t
#include <stddef.h>   // size_t

#include "../utils.h" // IsLittleEndian

#ifdef USE_SSE_INTRINSICS
#include "sse_vector.h"
#else
#include "scalar_vector.h"
#endif


// This uses an approximation for expf based on
// N. N. Schraudolph. "A fast, compact approximation of the exponential
// function." Neural Computation, 11(4), May 1999, pp.853-862.
//
// On the interval [-sqrt(87.33654), sqrt(87.33654)], the approximation for
// expf has a max relative error that is <= 3.55959567e-2. Outside of that
// interval we force the value to zero.
//
// Algorithm/Implementation Notes:
//  - this implementation came from
//    https://stackoverflow.com/a/47025627/4538758
//  - the magic value, 298765, was apparently chosen via a simple binary
//    search to try to minimize the maximum relative error for the expf(x)
//    function on the entire input [-87.33654, 88.72283]. Given that the
//    kernel only evaluates expf(x) over [-87.33654,0] this may be improved
//  - if more precison is needed, could potentially replace the linear
//    operation with a quadratic or cubic operation. For more details see
//    https://stackoverflow.com/a/50425370/4538758
//  - this algorithm will not work correctly on big endian machines
//
// Vectorization Note:
//  - Abstracting the masking operations to work in the case without scalars is
//    non-trivial. It's easier just to implement the kernel on a case-by-case
//    basis.
//  - to best understand the algorthim, consider the version without intrinsics
#define EXPF_SCHRAUDOLPH_MAX 87.33654f
#define EXPF_SCHRAUDOLPH_MAGIC_NUM 298765
#define EXPF_SCHRAUDOLPH_OFFSET (127 * (1 << 23) - EXPF_SCHRAUDOLPH_MAGIC_NUM)
// EXPF_SCHRAUDOLPH_SLOPE = -1*(1 << 23)/log(2)
#define EXPF_SCHRAUDOLPH_SLOPE -12102203.0f


// Multiply KERNEL_ARG_COEF by the argument before passing it into kernel
// For a Gaussian, it's equal to 1/sqrt(2)
#define KERNEL_ARG_COEF 0.70710677f
// Multiply KERNEL_NORM_COEF by the output of the kernel to normalize it
// For a Gaussian, it's equal to 1/sqrt(2*pi)
#define KERNEL_NORM_COEF 0.3989423f 


#ifdef USE_SSE_INTRINSICS

static inline __m128 kernel(__m128 u)
{
	// reminder: f32x4 is a type alias of __m128

	const __m128 slope = _mm_set1_ps(EXPF_SCHRAUDOLPH_SLOPE);
	const __m128i offset = _mm_set1_epi32 (EXPF_SCHRAUDOLPH_OFFSET);
	const __m128 max_expf_arg_mag = _mm_set1_ps(EXPF_SCHRAUDOLPH_MAX);

	__m128 u2 = _mm_mul_ps(u,u);

	// lanes in validity that are true, have all bits set to 1, while lanes
	// that are false have all bits set to 0
	__m128i validity = _mm_castps_si128(_mm_cmplt_ps(u2, max_expf_arg_mag));
	__m128i int_form = _mm_add_epi32(_mm_cvtps_epi32(_mm_mul_ps(slope,u2)),
					 offset);
	return  _mm_castsi128_ps (_mm_and_si128(validity,int_form));
}

#else

static inline f32x4 kernel(f32x4 u)
{
	const float slope = EXPF_SCHRAUDOLPH_SLOPE;
	const int32_t offset = EXPF_SCHRAUDOLPH_OFFSET;
	const float max_expf_arg_mag = EXPF_SCHRAUDOLPH_MAX;
	f32x4 out;
	for (int i= 0; i<4; i++){
		// The multiplication by `validity` masks the value to zero.
		// - If validity == 0, then all bits in the product will be 0,
		//   which
		//   is the exact floating point representation of 0.
		// - If validity == 1, then nothing changes.
		// If we multiplied by the mask after the conversion, there is
		// a chance we could have ended up with a NaN or inf when
		// reinterpretting the int as a float.
		float u2 = u.arr[i]*u.arr[i];
		int32_t validity = (u2 < max_expf_arg_mag);
		union {
			float f;
			int32_t i;
		} converter;
		converter.i = validity*(offset + (int32_t)(slope*u2));
		out.arr[i] = converter.f;
	}
	return out;
}
#endif /* USE_SSE_INTRINSICS */

int CheckCorrentrogramsProp(size_t winsize, size_t max_lag, size_t hopsize)
{
	if ((max_lag < 4) || (max_lag %4 != 0)){
		return -4;
	} else if ((winsize < 4) || (winsize %4 != 0)){
		return -5;
	} else if ((hopsize < 4) || (hopsize %4 != 0)){
		return -6;
	}

	return 0;
}

int CalcSummedLagCorrentrograms(const float * restrict x,
				const float * restrict bandwidths,
				size_t winsize, size_t max_lag,
				size_t hopsize, size_t n_win,
				float * restrict summed_acgrams)
{

	if (!IsLittleEndian()){
		// TODO: Write an implementation for big endian machines
		return -1;
	} else if ((size_t)x % 16 != 0) {
		return -2;
	}

	int arg_check = CheckCorrentrogramsProp(winsize, max_lag, hopsize);
	if (arg_check){
		return arg_check;
	}

	const float norm_coef = 0.3989423f;  // = 1/sqrt(2*pi)
	const float arg_coef =  0.70710677f; // = 1/sqrt(2)
	for (size_t win_ind = 0; win_ind < n_win; win_ind++){
		size_t win_start = win_ind*hopsize;

		const float inv_bandwidth = 1.f/bandwidths[win_ind];
		const float accum_coef = (inv_bandwidth * KERNEL_NORM_COEF /
					  (float)winsize);
		f32x4 dx_coef = broadcast_scalar_f32x4(KERNEL_ARG_COEF *
						       inv_bandwidth);

		f32x4 accum = broadcast_scalar_f32x4(0.f);

		// large holds a big number - it must be big enough that the
		// the result of a kernel evaluation involving it as one of the
		// terms results in a value of zero.
		f32x4 large = broadcast_scalar_f32x4(99.f);

		f32x4 next_align_seg = large;
		for (size_t i = 0; i <= winsize; i+=4){
			f32x4 cur_align_seg = next_align_seg;
			if (i ==winsize){
				next_align_seg = large;
			} else {
				next_align_seg = load_f32x4(x + i + win_start);
			}

			int jstart = (i > 0) ? 0 : 1;
			for (int j = jstart; j < 4; j++){
				f32x4 window_seg;

				// let cur_align_seg = [a,b,c,d] and
				//     next_align_seg = [e,f,g,h]
				if (j == 0){        // window_seg = [a,b,c,d]
					window_seg = lshift_extract_f32x4
						(cur_align_seg,
						 next_align_seg, 0);
				} else if (j == 1){ // window_seg = [b,c,d,e]
					window_seg = lshift_extract_f32x4
						(cur_align_seg,
						 next_align_seg, 1);
				} else if (j == 2){ // window_seg = [c,d,e,f]
					window_seg = lshift_extract_f32x4
						(cur_align_seg,
						 next_align_seg, 2);
				} else {           // window_seg = [d,e,f,g]
					window_seg = lshift_extract_f32x4(
						cur_align_seg,
						next_align_seg, 3);
				}

				// NOTE: when k=0, lagged_signal equals
				// next_align_seg 
				for (size_t k= 0; k<= max_lag - 4; k+=4){
					
					f32x4 lagged_signal =
						load_f32x4(x+i+k+win_start);
					f32x4 dx = sub_f32x4(window_seg,
							     lagged_signal);
					accum = add_f32x4(
						kernel(mul_f32x4(dx,dx_coef)),
						accum);
				}
			}
		}
		alignas(16) float accum_arr[4];
		store_f32x4(&accum_arr[0],accum);
		summed_acgrams[win_ind]
			+= accum_coef * ((accum_arr[0] + accum_arr[1]) + 
					 (accum_arr[2] + accum_arr[3]));
	}
	return 0;
}
