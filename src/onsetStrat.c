#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include "onsetsds.h"
#include "resample.h"
#include "onsetStrat.h"
#include "testOnset.h"
#include "simpleDetFunc.h"

OnsetStrategyFunc chooseOnsetStrategy(char* name){
	// this function returns the fundamental detection strategy named name
	// all names are case insensitive
	// this function returns NULL if the name is invalid

	OnsetStrategyFunc detectionStrategy;

	// Lets make name lowercased
	// https://stackoverflow.com/questions/2661766/c-convert-a-mixed-case-
	//string-to-all-lower-case

	for(int i = 0; name[i]; i++){
		name[i] = tolower(name[i]);
	}
	
	if (strcmp(name,"onsetsds")==0) {
		detectionStrategy = &OnsetsDSDetectionStrategy;
	} else if (strcmp(name, "TransientAlg")==0){
		detectionStrategy = &TransientDetectionStrategy;
	}
	else{
		detectionStrategy = NULL;
	}
	return detectionStrategy;
}

int OnsetsDSDetectionStrategy(float** AudioData, int size, int dftBlocksize, int samplerate, intList* onsets){
	int medspan = 15;
	int numBlocks = size / dftBlocksize;

	OnsetsDS ods; // An instance of the OnsetsDS struct
	float* block = malloc(sizeof(float) * dftBlocksize); //hold a block of data from AudioData to process

	enum onsetsds_odf_types odftype = ODS_ODF_RCOMPLEX; //various onset detectors available, ODS_ODF_RCOMPLEX should be the best
	float* odsdata = (float*) malloc(onsetsds_memneeded(odftype, dftBlocksize, medspan)); // Allocate contiguous memory ods needs for processing onset
	if(odsdata == NULL){
		printf("malloc error\n");
		fflush(NULL);
		free(block);
		return -1;
	}
	onsetsds_init(&ods, odsdata, ODS_FFT_FFTW3_R2C, odftype, dftBlocksize, medspan, samplerate);//initialise the OnsetsDS struct and its associated memory

	for(int i = 0; i < numBlocks; ++i){
		// Grab your 512-point, 50%-overlap, nicely-windowed FFT data, into block
		for (int j = 0; j < dftBlocksize; ++j){
			block[j] = (*AudioData)[ (i * dftBlocksize) + j ];
		}
		
		if(onsetsds_process(&ods, block)){
			//printf("new onset\n");
			if(intListAppend(onsets,i) != 1){//resize failed
				printf("Resizing onsets failed. Exitting.\n");
				free(block);
				free(ods.data);
				return -1;
			}
		}
	}

	free(block);
	free(ods.data); // Or free(odsdata), they point to the same thing in this case

	if(onsets->length == 0){ //if no onsets found, do not realloc here. Unable to realloc array to size 0
		return 0;
	}
	printf("realloc to size %ld\n", onsets->length*sizeof(int));
	if (intListShrink(onsets)!=1){
		printf("Resizing onsets failed. Exitting.\n");
		// intList was preallocated, we intentionally won't free it
		return -1;
	}

	return onsets->length;
}

void normalizeDetFunction(float ** detFunction, int length)
{
	float maxVal = -FLT_MAX;
	for(int i = 0; i < length; ++i){
		if(fabs((*detFunction)[i]) > maxVal){
			maxVal = fabs((*detFunction)[i]);
		}
	}
	//printf("maxVal: %f\n", maxVal);

	//instead of normalizing detfunction between -1 and 1, we normalize between -6.666 and 6.666, 
	//as each kernel we compare to ranges between +/- 6.666 (more precisly, +/- 1/0.15)
	maxVal = maxVal / 6.666f;
	for(int i = 0; i < length; ++i){
		(*detFunction)[i] = (*detFunction)[i] / maxVal;
		//printf("    index %d, time %f,     %f\n", i, i/200.0f, (*detFunction)[i]);
	}
}

int TransientDetectionStrategy(float** AudioData, int size, int dftBlocksize, int samplerate, intList* onsets)
{
	printf("in transientDetectionStrategy\n");

	//1. downsample audiodata to 11025 using libsamplerate
	int samplerateOld = samplerate;
	samplerate = 11025;
	float* ResampledAudio = NULL;
	float sampleRatio = samplerate/((float)samplerateOld);
	int RALength = Resample(AudioData, size, sampleRatio, &ResampledAudio);
	if(RALength == -1){
		return -1;
	}

	printf("resample complete\n");


	// 2+3 generate Detection Function from Gammatone Filter Bank

	// numchannels of gammatone filter. should always be 64
	int numChannels = 64;
	// min freq for ERB to determine central freqs of gammatone channels
	float minFreq = 80;
	// max freq for ERB to determine central freqs of gammatone channels
	float maxFreq = 4000;
	// length of the correntropy window. According to the paper, if
	// minFreq = 80 Hz, set to samplerate/80 
	int correntropyWinSize = samplerate/80;
	// hopsize, h, for detect func, in samples. Paper says 5ms
	// (samplerate/200). Assumed to be same as interval h where sigma is
	// optimized. 
	int interval = samplerate/200;
	// magic, grants three wishes
	float scaleFactor = powf(4./3.,0.2);
	// window size for sigma optimizer in numbers of samples. Paper suggests
	// 7s.
	int sigWindowSize = (samplerate*7);

	float* detectionFunction = NULL;
	int detectionFunctionLength =
		simpleDetFunctionCalculation(correntropyWinSize, interval,
					     scaleFactor, sigWindowSize,
					     samplerate, numChannels, minFreq,
					     maxFreq, ResampledAudio, RALength,
					     &detectionFunction);

	printf("detect func created\n");

	normalizeDetFunction(&detectionFunction, detectionFunctionLength);

	//4. generate transients
	int transientsLength = detectTransients(onsets, detectionFunction,
						detectionFunctionLength);
	if(transientsLength == -1){
		printf("detectTransients failed\n");
		free(ResampledAudio);
		free(detectionFunction);
		return -1;
	}

	printf("transients created\n");

	// convert onsets so that the value is not the index of
	// detectionFunction, but the sample of the original audio it occurred
	// onsets[i] * interval gives us the sample index in 11025 Hz, then we
	// convert to the original samplerate first significant frame instead
	// of first frame is the better way to do it, but do this for now.
	int* o_arr = onsets->array;
	for(int i = 0; i < transientsLength; ++i){
		//with samplerateOld of 44100, equivalent to arr[i] * 220
		o_arr[i] = (int)(o_arr[i] * interval
				 * (samplerateOld / (float)samplerate));
	}

	for(int k = 0; k < transientsLength; k+=2){
		printf("  %d - %d   (%dms - %dms)\n",
		       o_arr[k], o_arr[k+1],
		       (int)(o_arr[k]*(1000/(float)samplerateOld)),
		       (int)(o_arr[k+1]*(1000/(float)samplerateOld)) );
	}
	printf("done, %d notes found\n", transientsLength / 2);


	free(ResampledAudio);
	free(detectionFunction);
	return transientsLength;
}
