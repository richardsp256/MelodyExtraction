#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "samplerate.h"

int ResampledLength(int len, float sampleRatio)
{
	// guesses a length for the resampled audio
	// we can probably do better than a guess
	float result = ceilf(len*sampleRatio);
	if (result > (float)INT_MAX){
		return -1;
	}
	return (int)result;
}

int Resample(float* input, int len, float sampleRatio, float *output)
{
	/* Helper function for libsamplerate. 
	It resamples the input by the sampleRatio
	ex. a sampleRatio of 2 doubles the samplerate*/

	int success_code,result_length;

	int output_frames = ResampledLength(len, sampleRatio);
	if (output_frames < 0){
		return -1;
	}

	SRC_DATA *sampler = malloc(sizeof(SRC_DATA));
	sampler -> data_in  = input;
	sampler -> data_out = output;
	sampler -> input_frames = (long)len;
	sampler -> output_frames = (long)output_frames;
	sampler -> src_ratio = sampleRatio;

	success_code = src_simple(sampler, 0, 1);

	if (success_code != 0){
		printf("libsamplerate Error:\n %s\n",
		       src_strerror(success_code));
		result_length = -1;
	} else {
		result_length = (int)(sampler->output_frames_gen);
	}
	free(sampler);
	return result_length;
}


// ResampleAndAlloc resamples the input audio and allocates the memory for the
// output audio
int ResampleAndAlloc(float** input, int len, float sampleRatio, float **output)
{
	int guessed_resample_length = ResampledLength(len, sampleRatio);
	if (guessed_resample_length <= 0){
		return guessed_resample_length;
	}

	(*output) = malloc(sizeof(float) * guessed_resample_length);
	int result_length = Resample(*input, len, sampleRatio, *output);
	if (result_length < 0){
		free(*output);
	}
	return result_length;
}
