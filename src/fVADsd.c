/*
libfvad.so, libfvad.la, libdvad.so.0, and libfvad.so.0.0.0 are installed in /usr/local/lib

fvad.h is installed in /usr/local/include
*/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "fvad.h"
#include "resample.h"
#include "fVADsd.h"
#include "winSampleConv.h"

int fVADSilenceDetection(float** AudioData,int sample_rate, int mode,
			 int frameLength, int spacing, int length,
			 int** activityRanges)
{
	// this function determines the entries of activityRanges and returns
	// the length of activityRanges

	// mode can only be 0, 1, 2, or 3. higher is more restricted in reporting speech

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

		float* output = NULL;
		float sampleRatio = 8000.f/sample_rate;
		int output_length = ResampleAndAlloc(*AudioData, length, sampleRatio, &output);
		if(output_length == -1){
			return -1;
		}
		
		activityRangesLength = vadHelper(output,
					    8000, mode, frameLength,
					    spacing,
					    output_length,
					    activityRanges);

		free(output);

		if (activityRangesLength != -1){
			// We convert from the indices of the samples at
			// 8000 Hz to the indices of the samples at the original rate.
			WindowsToSamples(*activityRanges, activityRangesLength,
					 sample_rate);
		}
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
	      int spacing, int length, int** activityRanges){
	//data - array holding the audio data
	//samplerate - the samplerate of data
	//mode - mode of the vad, from 0 to 3, higher = more strict, more likely to say there is silence
	//framelength - the length of one frame, in milliseconds
	//spacing - the spacing between frames, in milliseconds
	//length - length of data
	//activityRanges - where the resulting silence info will be stored

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

	// calculate the number of Frames evaluated by VAD
	numFrames = posIntCeilDiv(length,spacingSamples);
	
	printf("length: %d\n", length);
	printf("number of Frames: %d\n", numFrames);
	printf("samplerate: %d\n", sample_rate);
	printf("frameLength: %d ms\n", frameLength);
	printf("frameLengthSamples: %d\n", frameLengthSamples);
	printf("spacing: %d ms\n", spacing);
	printf("spacingSamples: %d\n", spacingSamples);

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
		convertSamples(data, i*spacingSamples, frameLengthSamples, buffer, length);
		
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
					printf("reallocActivityRanges failed. Exitting.\n");
					fvad_free(vad);
					free(buffer);
					return -1;
				}
			}
			
			(*activityRanges)[j] = winStartRepSampleIndex(spacingSamples,
								      frameLengthSamples,
								      length,
								      i);			
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
		(*activityRanges)[j] = winStopRepSampleIndex(spacingSamples,
							     frameLengthSamples,
							     length,
							     i-1);
		j++;
	}

	// clean up
	fvad_free(vad);
	free(buffer);

	// resize activityRanges down to length j
	temp = realloc(*activityRanges,j*sizeof(int));
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

	temp = realloc(*activityRanges,newAcLength*sizeof(int));
	if (temp!=NULL){
		*activityRanges=temp;
	} else {
		printf("Resizing activityRanges failed. Exitting.\n");
		free(*activityRanges);
		return -1;
	}

	return newAcLength;
}

void WindowsToSamples(int* windows, int length, int sample_rate)
{
	// converts an array of indices of windows to the index of the first
	// sample in each window in place
	
	// winInt is in terms of ms
	// we round down to the whole number of samples
	double temp;
	for (int i = 0; i <length;i++){
		temp = (double)windows[i]*sample_rate / 8000.0; 
		windows[i] = (int)floor(temp);
	}
}
