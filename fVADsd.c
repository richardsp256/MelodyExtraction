/*
libfvad.so, libfvad.la, libdvad.so.0, and libfvad.so.0.0.0 are installed in /usr/local/lib

fvad.h is installed in /usr/local/include
*/

#include <stdlib.h>
#include <stdio.h>
#include <fvad.h>
#include <samplerate.h>
#include "fVADsd.h"


int fVADSilenceDetection(float** AudioData,int sample_rate, int mode,
			 int frameLength, int spacing, int length,
			 int* activityRanges)
{
	// this function determines the entries of activityRanges and returns
	// the length of activityRanges

	// mode can only be 0, 1, 2, or 3

	// frameLength has units of milliseconds - it can only be set to:
	// 10, 20, or 30

	// spacing has units of milliseconds

	int activityRangesLength;
	
	// Here we will check the sample_rate
	// The only allowed sample_rate values are 8000,16000,32000,48000
	
	if (sample_rate != 8000 && sample_rate != 16000 && sample_rate != 32000
	    && sample_rate != 48000) {
		
		// we will resample. Since higher rates are resampled down to
		// 8000 within the program, we will just resample down to 8000

		// should probably check that sample rate is higher than 8000

		// I will probably use Secret Rabbit Code (aka libsamplerate)
		SRC_DATA *resampleData = malloc(sizeof(SRC_DATA));
		resampleData -> data_in = (*AudioData);
		// should try to calculate the max number of frames
		resampleData -> data_out = malloc(sizeof(float) * length);
		// the following 2 values are set under the assumption that we
		// only have 1 channel
		resampleData -> input_frames = (long)length;
		resampleData -> output_frames = (long)length;
		resampleData -> src_ratio = 8000./((double)sample_rate);

		// I do not need need to set up input_frames_used,
		// output_frames_used, end_of_input

		// the second argument in src_simple corresponds to the
		// converter_type. By default I have set it to the best
		// quality
		int success_code;
		success_code = src_simple(resampleData, 0, 1);
		
		if (success_code != 0){
			printf("libsamplerate Error:\n %s\n",
			       src_strerror(success_code));
			activityRangesLength = -1;
		} else {
			activityRangesLength=vadHelper(resampleData -> data_out,
						    8000, mode, frameLength,
						    spacing,
						    resampleData->output_frames,
						    activityRanges);
		}
		free(resampleData -> data_out);
		free(resampleData);
	} else {
		activityRangesLength = vadHelper((*AudioData), sample_rate, mode,
					      frameLength, spacing, length,
					      activityRanges);
	}

	return activityRangesLength;
}




void convertSamples(float *inputData, int start, int frameLengthSamples,
		    int16_t *buffer, int length)
{
	// Converts Audio samples from floats to ints
	// follows the example from libfvad
	if (start + frameLengthSamples < length){
		for(int i = 0; i < frameLengthSamples; i++){
			buffer[i] = (int16_t)(inputData[start + i] * INT16_MAX);
		}
	} else {
		for(int i = 0; i <length -start; i++){
			buffer[i] = (int16_t)(inputData[start + i] * INT16_MAX);
		}
		for(int i = length-start; i <frameLengthSamples; i++){
			buffer[i] = 0;
		}
	}
}

int posIntCeilDiv(int x,int y){
	// performs ceiling division on positive integers - ceil(x/y)
	// https://stackoverflow.com/questions/2745074/fast-ceiling-of-an-
	// integer-division-in-c-c
	return 1 + ((x-1)/y);
}


int vadHelper(float* data,int sample_rate, int mode, int frameLength,
	      int spacing, int length, int* activityRanges){
	// this function actually runs VAD
	// conver should be converted to number of samples

	// this function determines the entries of activityRanges and returns
	// the length of activityRanges
	
	Fvad *vad;
	int16_t *buffer;
	int i, j, vadResult, prevResult, activityRangesLength;
	int frameLengthSamples, spacingSamples;

	// frameLengthSamples is just the frameLength in units of samples
	frameLengthSamples = (frameLength * sample_rate)/1000;

	// likewise, spacingSamples is just spacing in units of samples
	spacingSamples = (spacing * sample_rate)/1000;
	
	buffer = malloc(sizeof(int16_t) * frameLengthSamples);


	// for now will set activityRangesLength to the maximum possible length
	// - in future we will grow the arrays

	// the maximum possible length of activityRanges is
	// 2 * ceil(number of frames/2)
	// and number of fames is ceil(length/spacingSamples)
	activityRangesLength = 2 * posIntCeilDiv(posIntCeilDiv(length,
							       spacingSamples),
						 2);
	activityRanges = malloc(sizeof(int)*activityRangesLength);
	
	// in activityRanges, the values at even indices are indices of frame
	// where a region of contiguous voice activity starts and the following
	// values are always the index of the frame after the final frame in
	// the contiguous voice activity region
	
	// intialize a vad instance
	vad = fvad_new();

	fvad_set_mode(vad,mode);
	fvad_set_sample_rate(vad,sample_rate);


	prevResult = 0;
	j = 0;
	for (i=0; i<length; i+=spacingSamples){
		// copy to buffer and convert to int16
		convertSamples(data, i, frameLengthSamples,buffer, length);
		
		// process the data in the buffer
		vadResult = fvad_process(vad, buffer, frameLengthSamples);
		
		// might want to check that vadResult is 0 or 1 until we are
		// confident that we have ironed out all the kinks
		
		if (vadResult != prevResult){
			// this means that either voice activity started or
			// ended between the current frame and the previous
			// frame
			activityRanges[j] = i;
			j++;
		}
		prevResult = vadResult;
	}

	// now we just need to handle the last frame
	if (prevResult == 1){
		activityRanges[j] = i;
		j++;
	}

	// resize activityRanges down to length j
	
	// clean up
	fvad_free(vad);
	free(buffer);
	return j;
}


int reallocActivityRanges(int* activityRanges, int acLength, int length){
	// we will increase the size of activityRanges and return the new
	// length size of activityRanges

	// the max number of silence ranges is ceil(length/2)
	// thus, the max length of activityRanges is 2*ceil(length/2)

	// lets initially start with an array of size ceil(length/2)/5
	// then we double size until we hit 8 * ceil(length/2)/5 at which point,
	// we jump to 2*ceil(length/2)
	
	
	// you can do ceiling division for integers >0 ceil(x/y) with
	// 1 + ((x-1) / y)
	// this works since we will always have non-zero sizes
	return 0;
}
