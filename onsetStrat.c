#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "onsetsds.h"
#include "onsetStrat.h"

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
	} else {
		detectionStrategy = NULL;
	}
	return detectionStrategy;
}

void OnsetsDSDetectionStrategy(float** AudioData, int size, int dftBlocksize, int spacing, int samplerate){
	/*
	 this function was added for this project, and not poart of the 
	 original code by DanStowell. It returns a float array containing the timestamp in milliseconds of each onset
	*/
	 int numBlocks = size / dftBlocksize;
	 float delta = (spacing * 1000) / (float)samplerate; //time in ms between start of each block
	 printf("onset delta %f\n", delta);
	
	// An instance of the OnsetsDS struct, declared/allocated somewhere in your code, however you want to do it.
	OnsetsDS ods;

	// There are various types of onset detector available, we must choose one
	enum onsetsds_odf_types odftype = ODS_ODF_RCOMPLEX;

	// Allocate contiguous memory using malloc or whatever is reasonable.
	float* odsdata = (float*) malloc( onsetsds_memneeded(odftype, 512, 11) );

	// Now initialise the OnsetsDS struct and its associated memory
	onsetsds_init(&ods, odsdata, ODS_FFT_FFTW3_HC, odftype, 512, 11, samplerate);

	float* block = malloc(sizeof(float) * dftBlocksize);
	for(int i = 0; i < numBlocks; ++i){
		// Grab your 512-point, 50%-overlap, nicely-windowed FFT data, into block
		for (int j = 0; j < dftBlocksize; ++j){
			block[j] = (*AudioData)[ (i * dftBlocksize) + j ];
		}
		
		//will return true when there's an onset, false otherwise
		if(onsetsds_process(&ods, block)){
			printf("onset detected at %d\n", (int)(delta*i));
		}
	}

	free(block);

	free(ods.data); // Or free(odsdata), they point to the same thing in this case
}