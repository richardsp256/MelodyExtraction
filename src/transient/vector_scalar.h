/// @file     vector_scalar.h
/// @brief    [\ref transient] Fallback implementation of SIMD Vector type
///     without intrinsics
///
/// This file should be used when there is no viable alternative backend
/// including SIMD intrinsics. This backend does facillitate autovectorization
///
/// This header file should only be included in source files. To avoid bloating
/// the library/binary, it should NOT be included in header files (the static
/// functions will be separately implemented in every translation where this
/// was included)

#include <stdalign.h>
#include <assert.h>
#include <stdint.h>

#ifndef SCALAR_VECTOR_H
#define SCALAR_VECTOR_H

inline const char* vector_backend_name() { return "scalar"; }

typedef struct f32x4 {
	alignas(16) float arr[4];
} f32x4;

static inline f32x4 load_f32x4(const float * p){
	f32x4 out;
	for (int i = 0; i < 4; i++){
		out.arr[i] = p[i];
	}
	return out;
}

static inline void store_f32x4(float * p, f32x4 a){
	for (int i = 0; i < 4; i++){
		p[i] = a.arr[i];
	}
}

static inline f32x4 broadcast_scalar_f32x4(float scalar){
	f32x4 out;
	for (int i = 0; i < 4; i++){
		out.arr[i] = scalar;
	}
	return out;
}

static inline f32x4 add_f32x4(f32x4 a, f32x4 b){
	f32x4 out;
	for (int i = 0; i < 4; i++){ 
		out.arr[i] = a.arr[i] + b.arr[i];
	}
	return out;
}

static inline f32x4 sub_f32x4(f32x4 a, f32x4 b){
	f32x4 out;
	for (int i = 0; i < 4; i++){ 
		out.arr[i] = a.arr[i] - b.arr[i];
	}
	return out;
}

static inline f32x4 mul_f32x4(f32x4 a, f32x4 b){
	f32x4 out;
	for (int i = 0; i < 4; i++){ 
		out.arr[i] = a.arr[i] * b.arr[i];
	}
	return out;
}

static inline f32x4 lshift_extract_f32x4(f32x4 left, f32x4 right, int nlanes){
	f32x4 out;
	assert(nlanes < 4 && nlanes >= 0);
	switch(nlanes){
	case 0:
		out = left;
                break;
        case 1:
		out.arr[0] = left.arr[1];
                out.arr[1] = left.arr[2];
                out.arr[2] = left.arr[3];
                out.arr[3] = right.arr[0];
		break;
        case 2:
		out.arr[0] = left.arr[2];
                out.arr[1] = left.arr[3];
                out.arr[2] = right.arr[0];
                out.arr[3] = right.arr[1];
                break;
        case 3:
		out.arr[0] = left.arr[3];
                out.arr[1] = right.arr[0];
                out.arr[2] = right.arr[1];
                out.arr[3] = right.arr[2];
                break;
	}
	return out;
}

#endif /*SCALAR_VECTOR_H*/
