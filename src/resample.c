/// @file     resample.c
/// @brief    Implementation of resampling functions

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "samplerate.h"
#include "errors.h"

int ResampledLength(int len, float sampleRatio)
{
	// use double to maintain percision closer to INT_MAX
	double result = ceil(len*(double)sampleRatio - 1);
	if (result <= 0 || result >= (double)INT_MAX){
		return ME_BAD_SAMPLERATIO;
	}
	return (int)result;
}

int Resample(const float* input, int len, float sampleRatio, float *output)
{
	/* It resamples the input by the sampleRatio
	ex. a sampleRatio of 2 doubles the samplerate*/

	int output_frames = ResampledLength(len, sampleRatio);
	if (output_frames < 0){
		return output_frames;
	}

	SRC_DATA sampler;
	sampler.data_in = input;
	sampler.data_out = output;
	sampler.input_frames = (long)len;
	sampler.output_frames = (long)output_frames;
	sampler.src_ratio = sampleRatio;

	int success_code = src_simple(&sampler, 0, 1);

	int result_length;
	if (success_code != 0){
		printf("libsamplerate Error: %s\n",
		       src_strerror(success_code));
		result_length = ME_LIBSAMPLERATE_ERROR;
	} else if (len != sampler.input_frames_used){
		printf("resample Error: input not all used\n");
		result_length = ME_UNUSED_RESAMPLE_INPUT;
	} else if (output_frames != sampler.output_frames_gen){
		printf("resample Error: output sized incorrectly\n");
		result_length = ME_RESAMPLE_OUTPUT_SIZE_ERROR;
	} else {
		result_length = (int)(sampler.output_frames_gen);
	}

	return result_length;
}

int ResampleAndAlloc(const float* input, int len, float sampleRatio,
		     float **output)
{
	int guessed_resample_length = ResampledLength(len, sampleRatio);
	if (guessed_resample_length < 0){
		return guessed_resample_length;
	}

	(*output) = malloc(sizeof(float) * guessed_resample_length);
	if((*output) == NULL){
		return ME_MALLOC_FAILURE;
	}
	int result_length = Resample(input, len, sampleRatio, *output);
	if (result_length < 0){
		free(*output);
		*output = NULL; // avoid dangling pointer
	}
	return result_length;
}
