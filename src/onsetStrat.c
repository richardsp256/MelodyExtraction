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


// Note: This function signature is a little hacky. DftBlocksize is never used
// and onsets contains all transients (including onsets and offsets)
// May want to rename this something like "PairwiseTransientStrategy"
int TransientDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			       int samplerate, intList* onsets)
{
	printf("in transientDetectionStrategy\n");

	//downsample audiodata to 11025 using libsamplerate
	int samplerateOld = samplerate;
	samplerate = 11025;
	float* ResampledAudio = NULL;
	float sampleRatio = samplerate/((float)samplerateOld);
	int RALength = ResampleAndAlloc(AudioData, size, sampleRatio, &ResampledAudio);
	if(RALength == -1){
		return -1;
	}

	printf("resample complete\n");

	int transientsLength = 
		pairwiseTransientDetection(ResampledAudio, RALength,
					   samplerate, onsets);

	if(transientsLength <= 0){
		free(ResampledAudio);
		return transientsLength;
	}

	// convert onsets so that the sample corresponds to the old sample rate
	// rather than the resampled audio samplerate
	int* o_arr = onsets->array;
	for(int i = 0; i < transientsLength; ++i){
		o_arr[i] *= (int)((samplerateOld / (float)samplerate));
	}

	for(int k = 0; k < transientsLength; k+=2){
		printf("  %d - %d   (%dms - %dms)\n",
		       o_arr[k], o_arr[k+1],
		       (int)(o_arr[k]*(1000/(float)samplerateOld)),
		       (int)(o_arr[k+1]*(1000/(float)samplerateOld)) );
	}
	printf("done, %d notes found\n", transientsLength / 2);


	free(ResampledAudio);
	return transientsLength;
}
