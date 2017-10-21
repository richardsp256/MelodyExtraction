#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "onsetsds.h"
#include "samplerate.h"
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

int OnsetsDSDetectionStrategy(float** AudioData, int size, int dftBlocksize, int samplerate, int** onsets){
	int medspan = 15;
	int numBlocks = size / dftBlocksize;

	OnsetsDS ods; // An instance of the OnsetsDS struct
	float* block = malloc(sizeof(float) * dftBlocksize); //hold a block of data from AudioData to process
	int onsets_size = 10; //initial size for onsets array. we will realloc for more space as needed
	int onsets_index = 0;

	enum onsetsds_odf_types odftype = ODS_ODF_RCOMPLEX; //various onset detectors available, ODS_ODF_RCOMPLEX should be the best
	float* odsdata = (float*) malloc(onsetsds_memneeded(odftype, dftBlocksize, medspan)); // Allocate contiguous memory ods needs for processing onset
	if(odsdata == NULL){
		printf("malloc error\n");
		fflush(NULL);
		free(block);
		return -1;
	}
	onsetsds_init(&ods, odsdata, ODS_FFT_FFTW3_R2C, odftype, dftBlocksize, medspan, samplerate);//initialise the OnsetsDS struct and its associated memory

	(*onsets) = malloc(sizeof(int)*onsets_size);
	
	for(int i = 0; i < numBlocks; ++i){
		// Grab your 512-point, 50%-overlap, nicely-windowed FFT data, into block
		for (int j = 0; j < dftBlocksize; ++j){
			block[j] = (*AudioData)[ (i * dftBlocksize) + j ];
		}
		
		if(onsetsds_process(&ods, block)){
			//printf("new onset\n");
			AddOnsetAt(onsets, &onsets_size, i, onsets_index);
			if(onsets_size == -1){ //resize failed
				printf("Resizing onsets failed. Exitting.\n");
				free(block);
				free(ods.data);
				return -1;
			}

			onsets_index++;
		}
	}

	free(block);
	free(ods.data); // Or free(odsdata), they point to the same thing in this case

	if(onsets_index == 0){ //if no onsets found, do not realloc here. Unable to realloc array to size 0
		return onsets_index;
	}
	printf("realloc to size %ld\n", onsets_index*sizeof(int));
	int* temp = realloc((*onsets),onsets_index*sizeof(int));
	if (temp != NULL){
		(*onsets) = temp;
	} else {
		printf("Resizing onsets failed. Exitting.\n");
		free(*onsets);
		return -1;
	}
	
	return onsets_index;
}

//adds [value] to [onsets] at index [index]. if the index is out of bounds, resize the array.
void AddOnsetAt(int** onsets, int* size, int value, int index ){
	if(index >= (*size)){ //out of space in onsets array
		(*size) *= 2;
		printf("  new size %d\n", (*size));
		printf("realloc to size %ld\n", (*size)*sizeof(int));
		int* temp = realloc((*onsets),(*size)*sizeof(int));
		if(temp != NULL){
			(*onsets) = temp;
		} else { //realloc failed
			free(*onsets);
			(*size) = -1;
			return;
		}
	}
	(*onsets)[index] = value;
}

int Resample(float** data, float **resampleData, int length, int samplerateOld, int samplerateNew)
{
	SRC_DATA *resampler = malloc(sizeof(SRC_DATA));

	resampler -> data_in = (*data);
	resampler -> data_out = malloc(sizeof(float) * length); //length is larger than needed
	resampler -> input_frames = (long)length;
	resampler -> output_frames = (long)length;
	resampler -> src_ratio = samplerateNew/((double)samplerateOld);

	int response = src_simple(resampler, 0, 1);

	if (response != 0){ //conversion failed
		free(resampler -> data_out);
		free(resampler);
		return -1;
	}

	(*resampleData) = resampler -> data_out;
	int RALength = resampler -> output_frames_gen;
	free(resampler);

	return RALength;
}

int TransientDetectionStrategy(float** AudioData, int size, int dftBlocksize, int samplerate, int** onsets)
{
	printf("in transientDetectionStrategy\n");

	//1. downsample audiodata to 11025 using libsamplerate
	int samplerateOld = samplerate;
	samplerate = 11025;
	float* ResampledAudio = NULL;
	int RALength = Resample(AudioData, &ResampledAudio, size, samplerateOld, samplerate);
	if(RALength == -1){
		return -1;
	}

	printf("resample complete\n");


	//2+3 generate Detection Function from Gammatone Filter Bank
	int numChannels = 64; //numchannels of gammatone filter. should always be 64
	float minFreq = 80; //min freq for ERB to determine central freqs of gammatone channels
	float maxFreq = 4000; //max freq for ERB to determine central freqs of gammatone channels
	int correntropyWinSize = samplerate/80; //length of the correntropy window. According to the paper, if minFreq = 80 Hz, set to samplerate/80 
	int interval = samplerate/200;//hopsize, h, for detect func, in samples. Paper says 5ms (samplerate/200). Assumed to be same as interval h where sigma is optimized. 
	float scaleFactor = powf(4./3.,0.2); //magic, grants three wishes
	int sigWindowSize = (samplerate*7)/interval; //window size for sigma optimizer in numbers of intervals. Paper suggests 7s.

	float* detectionFunction = NULL;
	int detectionFunctionLength = simpleDetFunctionCalculation(correntropyWinSize, interval, scaleFactor, sigWindowSize,
				     samplerate, numChannels, minFreq, maxFreq,
				     ResampledAudio, RALength, &detectionFunction);

	printf("detect func created\n");

	//4. generate transients
	int transientsLength = detectTransients(onsets, detectionFunction, detectionFunctionLength);
	if(transientsLength == -1){
		printf("detectTransients failed\n");
		free(ResampledAudio);
		free(detectionFunction);
		return -1;
	}

	printf("transients created\n");

	//convert onsets so that the value is not the index of detectionFunction, but the sample of the original audio it occurred
	//onsets[i] * interval gives us the sample index in 11025 Hz, then we convert to the original samplerate
	//first significant frame instead of first frame is the better way to do it, but do this for now.
	for(int i = 0; i < transientsLength; ++i){
		(*onsets)[i] = (int)( (*onsets)[i] * interval * (samplerateOld / (float)samplerate));  //with samplerateOld of 44100, equivalent to onsets[i] * 220
	}

	for(int k = 0; k < transientsLength; k+=2){
		printf("  %d - %d   (%dms - %dms)\n", (*onsets)[k], (*onsets)[k+1], (int)((*onsets)[k]*(1000/(float)samplerateOld)), (int)((*onsets)[k+1]*(1000/(float)samplerateOld)) );
	}
	printf("done, %d notes found\n", transientsLength / 2);


	free(ResampledAudio);
	free(detectionFunction);
	return transientsLength;
}
