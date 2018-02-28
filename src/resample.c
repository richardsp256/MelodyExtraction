#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "samplerate.h"

int Resample(float** input, int len, float sampleRatio, float **output)
{
	/* Helper function for libsamplerate. 
	It resamples the input by the sampleRatio
	ex. a sampleRatio of 2 doubles the samplerate*/

	int success_code,result_length;

	(*output) = malloc(sizeof(float)* (int)(len*sampleRatio)); 
	SRC_DATA *sampler = malloc(sizeof(SRC_DATA));
	sampler -> data_in = (*input);
	sampler -> data_out = (*output);
	sampler -> input_frames = (long)len;
	sampler -> output_frames = (long)(len*sampleRatio);
	sampler -> src_ratio = sampleRatio;

	success_code = src_simple(sampler, 0, 1);

	if (success_code != 0){
		printf("libsamplerate Error:\n %s\n",
		       src_strerror(success_code));
		free(output);
		free(sampler);
		return -1;
	}

	result_length = (int)(sampler->output_frames_gen);
	printf("resample %f: %d %d\n", sampleRatio, result_length, (int)(len*sampleRatio));
	free(sampler);
	return result_length;
}