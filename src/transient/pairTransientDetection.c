#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <limits.h> // FLT_MAX
#include <math.h> // fabs
#include <stdbool.h>
#include <float.h>

#include "pairTransientDetection.h"
#include "simpleDetFunc.h"
#include "../errors.h"

/// Frees the memory holding the preallocated kernels
static void freeKernels(float** kernels, int numKernels){
	for(int i = 0; i < numKernels; ++i){
		free(kernels[i]);
	}
	free(kernels);
}

static const float MINZ = -0.99999f;
static const float MAXZ = 0.99999f;
static const float DOUBLEMAXZ = 1.99998f;
static const float SHARPNESS_A = 0.15;

/// Allocate memory to hold the kernel functions and initialize the values
///
/// The kernel function is given by the function `Lambda(z) = z/(1 + a - |z|)`.
/// Here, `a` is called the "sharpness" and typically has a value of `0.15`.
/// To sample a kernel of length m, we assign each index an evenly spaced
/// value of `z` from `(-1 + 1e-5)` through `(1-e-5)` and use it to evaluate
/// `Lambda(z)`
///
/// @note
/// For sufficiently large `numKernels`, it would make sense to allocate a
/// single buffer to hold all kernels.
static float** GenKernels(int minKLen, int numKernels)
{
	if(numKernels <= 0){
		return NULL;
	}
	float** kernels = malloc(numKernels * sizeof(float*));
	for(int i = 0; i < numKernels; i++){
		int kernelLen = minKLen + i;
		kernels[i] = malloc(kernelLen * sizeof(float));
		if (kernels[i] == NULL){
			freeKernels(kernels,i-1);
			return NULL;
		}
		for(int j = 0; j < kernelLen; ++j){
			float z = ((DOUBLEMAXZ * j) / (kernelLen - 1)) + MINZ;
			kernels[i][j] = z/(1. + SHARPNESS_A - fabs(z));
		}
	}
	return kernels;
}

/// Computes the fitness from fitting a kernel for an onset or offset to a
/// windowed segment of the detection function.
///
/// Equation 7 of the paper indicates that the fitness of for some segment of
/// length m is given by
///       `m^-k * Sum((Lambda[i] - window[i])^2)`
/// where the sum runs from `i = 0` through `i = m-1`.
///
/// The difference between fitting onsets and offsets is that the kernel must
/// be multiplied by -1 to fit the former.
///
/// Note there appears to be some disaggreement in the paper about the sign of
/// the kernel for onsets. The equation for the onset fitness, eqn 6, suggests
/// that kernels the ith onset (offset) should be positive (negative). In
/// contrast, Fig 4 suggests that the signs should be flipped. Fig 4 suggests
/// the latter is correct.
static float CalcFitness(const float* kernel, const float* window, int len,
			 bool onset)
{
	float coef = (onset) ? -1.f: 1.f;
	float sum = 0.0f;
	for(int i = 0; i < len; ++i){
		float diff = (coef*kernel[i] - window[i]);
		sum += diff*diff;
	}
	return sum/len;
}

/// Computes the length of the best fitting kernel
static int BestFittingKernel(const float * detFunc, int len, int lastFitInd,
			     float ** kernels, int minkernel, int maxkernel,
			     bool onset)
{
	const float* winStart = detFunc + lastFitInd;
	int maxLength = maxkernel < (len - lastFitInd) ?
		maxkernel : (len - lastFitInd);

	float bestFitness = FLT_MAX;
	int bestLength = -1;

	for(int kLength = minkernel; kLength < maxLength; ++kLength){
		float curFitness = CalcFitness(kernels[kLength - minkernel],
					       winStart, kLength, onset);
		if(curFitness < bestFitness){
			bestFitness = curFitness;
			bestLength = kLength;
		}
	}
	if (bestLength == -1){
		return ME_BAD_KERNEL_FIT;
	} else{
		return bestLength;
	}
}


/// Normalize the detection function, in-place, such that the it has a similiar
/// range of values as the kernels. Basically we divide the entries by the
/// product of `SHARPNESS_A` and the max magnitude of the detection function.
///
/// @note
/// It remains somewhat unclear how necessary this actually is
static int normalizeDetFunction(float * detFunction, int length)
{
	float maxVal = -FLT_MAX;
	for(int i = 0; i < length; ++i){
		if(fabs(detFunction[i]) > maxVal){
			maxVal = fabs(detFunction[i]);
		}
	}
	if (maxVal == 0.){
		return ME_ALL_NULL_DETFUNC;
	}

	float factor = 1.f /(maxVal * SHARPNESS_A);
	for(int i = 0; i < length; ++i){
		detFunction[i] = detFunction[i] * factor;
	}
	return ME_SUCCESS;
}


int detectTransients(float* detection_func, int len, intList* transients){

	// renormalize the detection function.
	int tmp_rslt = normalizeDetFunction(detection_func, len);
	if (tmp_rslt != ME_SUCCESS){
		return tmp_rslt;
	}

	const int minkernel = 4;    // =20 ms for correntropy hopsize h of 5 ms
	const int maxkernel = 1500; // =7.5 s for correntropy hopsize h of 5 ms

	printf("detection with len %d, minK %d, maxK %d\n", len, minkernel, maxkernel);

	int numKernels = maxkernel - minkernel + 1;
	// Generate all of the kernels
	float** Kernels = GenKernels(minkernel, numKernels);
	if (Kernels == NULL){
		return ME_MALLOC_FAILURE;
	}

	int detect_index = 0;
	int lastpossibleOnset = len-minkernel; //last possible onset index bc offset requires at least minkernel indices.

	int i =-1;
	while(detect_index < lastpossibleOnset){
		const bool fit_onset = ((++i) % 2) == 0;
		int bestKLen = BestFittingKernel(detection_func, len,
						 detect_index, Kernels,
						 minkernel, maxkernel, fit_onset);
		if (bestKLen < 0){
			freeKernels(Kernels, numKernels);
			return bestKLen;
		}
		detect_index += bestKLen;
		if(fit_onset && (detect_index >= lastpossibleOnset)){
			//onset detected too close to end for a corresponding offset, so we skip the onset and end.
			break;
		}
		int tmp_rslt = intListAppend(transients, detect_index);
		if(tmp_rslt != ME_SUCCESS){
			printf("Resizing transients failed. Exitting.\n");
			freeKernels(Kernels, numKernels);
			return tmp_rslt;
		}
	}

	freeKernels(Kernels, numKernels);

	// the transient detection algorithm, by its design, will (almost)
	// always have an extra false positive note at the end. we only go up
	// to transients->length-2 to remove this note
	transients->length -= 2;
	if (transients->length <= 0){ // no transients found
		transients->length = 0;
		// Don't attempt to shrink here. Unable to realloc array to
		// size 0 (also don't free it - intList was preallocated)
		return ME_NO_TRANSIENTS;
	}
	tmp_rslt = intListShrink(transients);
	if (tmp_rslt != ME_SUCCESS){
		printf("Resizing onsets failed. Exitting.\n");
		// intList was preallocated, we intentionally won't free it
		return tmp_rslt;
	}
	return transients->length;
}

static int coercePosMultipleOf4_(int arg, bool round_up){
	if ((arg <=0) || (arg % 4) == 0){
		return arg;
	} else if (arg < 4) {
		return 4;
	} else if (round_up) {
		return 4*((arg/4) + 1);
	} else {
		return 4*(arg/4);
	}
}

int pairwiseTransientDetection(float *audioData, int size, int samplerate,
			       intList* transients){

	// use parameters suggested by paper
	int numChannels = 64;
	float minFreq = 80.f; // 80 Hz
	float maxFreq = 4000.f; // 4000 Hz
	int correntropyWinSize = samplerate/80; // assumes minFreq=80
	if ((samplerate % 80) == 0) {
		correntropyWinSize++;
	}
	int interval = samplerate/200; // 5ms
	float scaleFactor = powf(4./3.,0.2); // magic, grants three wishes
	                                     // (Silverman's rule of thumb)
	int sigWindowSize = (samplerate*7); // 7s

	// implementation requirement for correntropyWinSize & interval:
	correntropyWinSize = coercePosMultipleOf4_(correntropyWinSize, true);
	interval = coercePosMultipleOf4_(interval, false);

	// allocate the detectionFunction
	int detectionFunctionLength =
		computeDetFunctionLength(size, correntropyWinSize, interval);
	float* detectionFunction = malloc(sizeof(float) *
					  detectionFunctionLength);

	// compute the detectionFunction
	int rslt = simpleDetFunctionCalculation(correntropyWinSize, interval,
						scaleFactor, sigWindowSize,
						numChannels, minFreq, maxFreq,
						samplerate, size, audioData,
						detectionFunctionLength,
						detectionFunction);
	if (ME_SUCCESS != rslt){
		free(detectionFunction);
		return rslt;
	}

	printf("detect func computed\n");

	// idendify the transients
	int transientsLength = detectTransients(detectionFunction,
						detectionFunctionLength,
						transients);

	if(transientsLength == -1){
		printf("detectTransients failed\n");
		free(detectionFunction);
		return -1;
	}

	printf("transients created\n");

	// convert transients so that the values are not the indices of
	// detectionFunction, but corresponds to the frame of audioData
	int* t_arr = transients->array;
	for(int i = 0; i < transientsLength; ++i){
		t_arr[i] = t_arr[i] * interval;
	}

	return transientsLength;
}
