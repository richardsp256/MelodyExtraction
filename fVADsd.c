/*
libfvad.so, libfvad.la, libdvad.so.0, and libfvad.so.0.0.0 are installed in /usr/local/lib

fvad.h is installed in /usr/local/include
*/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <fvad.h>
#include <samplerate.h>
#include "fVADsd.h"


int fVADSilenceDetection(float** AudioData,int sample_rate, int mode,
			 int frameLength, int spacing, int length,
			 int** activityRanges)
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
			//printf("\nOriginal Sample Rate: %d Hz\n", sample_rate);
			//printf("Original Length: %d (# of samples)\n",
			//       length);
			//printf("New Sample Rate: %d Hz\n", 8000);
			//printf("New Length: %li (# of samples)\n\n",
			//      resampleData->output_frames_gen);
			
			activityRangesLength=vadHelper(resampleData -> data_out,
						    8000, mode, frameLength,
						    spacing,
						    resampleData->output_frames_gen,
						    activityRanges);
		}
		free(resampleData -> data_out);
		free(resampleData);
	} else {
		activityRangesLength = vadHelper((*AudioData), sample_rate, mode,
					      frameLength, spacing, length,
					      activityRanges);
	}
	if (activityRangesLength!=-1){
		// this scenario occurs if resizing *activityRanges succeeded
		// here we convert from the indices of the windows that were used
		// for VAD to the indices of the first samples in each window
		WindowsToSamples(*activityRanges, activityRangesLength,
				 spacing, sample_rate);
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
	      int spacing, int length, int** activityRanges){
	// this function actually runs VAD
	// conver should be converted to number of samples

	// this function determines the entries of activityRanges and returns
	// the length of activityRanges
	
	Fvad *vad;
	int16_t *buffer;
	int i, j, vadResult, prevResult, activityRangesLength;
	int frameLengthSamples, spacingSamples, numFrames;
	int *temp;

	// frameLengthSamples is just the frameLength in units of samples
	frameLengthSamples = (frameLength * sample_rate)/1000;

	// likewise, spacingSamples is just spacing in units of samples
	spacingSamples = (spacing * sample_rate)/1000;
	
	//printf("length: %d\n", length);
	//printf("samplerate: %d\n", sample_rate);
	//printf("frameLength: %d ms\n", frameLength);
	//printf("spacing: %d ms\n", spacing);
	//printf("frameLengthSamples: %d (# of samples)\n", frameLengthSamples);
	//printf("spacingSamples: %d (# of samples)\n", spacingSamples);

	// calculate the number of Frames evaluated by VAD
	numFrames = posIntCeilDiv(length,spacingSamples);
	//printf("number of Frames: %d\n", numFrames);

	// allocate memory for the buffer that will be processed by VAD
	buffer = malloc(sizeof(int16_t) * frameLengthSamples);	

	// allocate memory for activityRanges
	activityRangesLength = mallocActivityRanges(activityRanges, numFrames);

	// in activityRanges, the values at an even index is the index of the
	// frame where a region of contiguous voice activity begins and the
	// values at the following odd index of activityRanges is always the
	// index of the frame after the final frame in the contiguous voice
	// activity region
	
	// intialize a vad instance
	vad = fvad_new();

	fvad_set_mode(vad,mode);
	fvad_set_sample_rate(vad,sample_rate);


	prevResult = 0;
	j = 0;

	
	for (i=0; i<numFrames; i++){
		
		// copy to buffer and convert to int16
		convertSamples(data, i*spacingSamples, frameLengthSamples,buffer, length);
		
		// process the data in the buffer
		vadResult = fvad_process(vad, buffer, frameLengthSamples);
		
		
		if (vadResult != prevResult){
			// this means that either voice activity started or
			// ended between the current frame and the previous
			// frame

			// we need to check if we have enough room in
			// activityRanges
			if (j>=activityRangesLength){
				// we need to allocate more space
				activityRangesLength = reallocActivityRanges(activityRanges, activityRangesLength, numFrames);
				// we need to check that reallocating memory
				// was succesful
				if (activityRangesLength == -1){
					// clean up
					fvad_free(vad);
					free(buffer);
					return -1;
				}
			}

			(*activityRanges)[j] = i;
			j++;
		}
		prevResult = vadResult;
		
	}

	// now we just need to handle the last frame
	if (prevResult == 1){

		// we don't need to check if we have enough room in
		// activityRanges since it always has an even length and
		// there will always be an odd number of entries when we add this
		// last value
		*activityRanges[j] = i;
		j++;
	}

	// clean up
	fvad_free(vad);
	free(buffer);

	// resize activityRanges down to length j
	temp = realloc(*activityRanges,j);
	if (temp!=NULL){
		*activityRanges=temp;
	} else {
		printf("Resizing activityRanges failed. Exitting.\n");
		free(*activityRanges);
		return -1;
	}
	
	return j;
}

int mallocActivityRanges(int** activityRanges, int numFrames){
	// here we allocate memory for activityRanges and return the size
	// we will start with 1/20th of maximum size.
	
	// the maximum possible length of activityRanges is:
	// 2 * ceil(numFrames/2)
	// thus, the starting size will be: ceil(numFrames/2)/10

	int acLength;
	
	acLength = posIntCeilDiv(numFrames,2)/10;
	// we want acLength to be a positive even integer
	if (acLength == 0){
		acLength = 2;
	} else if (acLength % 2 == 1) {
		acLength++;
	}

	// now we actually allocate the memory
	*activityRanges = malloc(sizeof(int)*acLength);
	return acLength;
}

int reallocActivityRanges(int** activityRanges, int acLength, int numFrames){
	// we will increase the size of activityRanges and return the new
	// length size of activityRanges

	// activityRanges is initially set to be 1/20th of the maximum size
	// as we need more space, we double the size of activityRanges until it
	// exceeds the maximum size - at this point we set activityRanges to
	// the maximum size

	int *temp;
	int newAcLength, maxAcLength;

	// First, we calculate the maximum possible length of activityRanges
	// the maximum possible length of activityRanges is:
	// 2 * ceil(numFrames/2)
	maxAcLength = 2 * posIntCeilDiv(numFrames,2);

	// next we set newAcLength to min(acLength*2, maxAcLength)
	newAcLength = acLength *2;
	if (newAcLength >= maxAcLength){
		newAcLength = maxAcLength;
	}

	temp = realloc(*activityRanges,acLength);
	if (temp!=NULL){
		*activityRanges=temp;
	} else {
		printf("Resizing activityRanges failed. Exitting.\n");
		free(*activityRanges);
		return -1;
	}

	return newAcLength;
}

void WindowsToSamples(int* windows, int length, int winInt,
		      int sample_rate)
{
	// converts an array of indices of windows to the index of the first
	// sample in each window in place
	
	// winInt is in terms of ms
	// we round down to the whole number of samples
	double temp;
	for (int i = 0; i <length;i++){
		temp = ((double)winInt/1000.) * (double)windows[i]*sample_rate;
		windows[i] = (int)floor(temp);
	}
}
