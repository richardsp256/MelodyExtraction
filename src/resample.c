#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "samplerate.h"

int ResampledLength(int len, float sampleRatio)
{
	//use double to maintain percision closer to INT_MAX
	double result = ceil(len*(double)sampleRatio - 1);
	if (result <= 0 || result >= (double)INT_MAX){
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
		printf("libsamplerate Error: %s\n",
		       src_strerror(success_code));
		result_length = -1;
	} else if (len != sampler->input_frames_used){
		printf("resample Error: input not all used\n");
		result_length = -1;
	} else if (output_frames != sampler->output_frames_gen){
		printf("resample Error: output sized incorrectly\n");
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
		return -1;
	}

	(*output) = malloc(sizeof(float) * guessed_resample_length);
	if((*output) == NULL){
		return -1;
	}
	int result_length = Resample(*input, len, sampleRatio, *output);
	if (result_length < 0){
		free(*output);
	}
	return result_length;
}
