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

int OnsetsDSDetectionStrategy(float** AudioData, int size, int dftBlocksize, int samplerate, int** onsets){
	int numBlocks = size / dftBlocksize;

	OnsetsDS ods; // An instance of the OnsetsDS struct
	float* block = malloc(sizeof(float) * dftBlocksize); //hold a block of data from AudioData to process
	int onsets_size = 10; //initial size for onsets array. we will realloc for more space as needed
	int onsets_index = 0;

	enum onsetsds_odf_types odftype = ODS_ODF_RCOMPLEX; //various onset detectors available, ODS_ODF_RCOMPLEX should be the best
	float* odsdata = (float*) malloc(onsetsds_memneeded(odftype, 512, 11)); // Allocate contiguous memory ods needs for processing onset
	onsetsds_init(&ods, odsdata, ODS_FFT_FFTW3_R2C, odftype, 512, 11, samplerate);//initialise the OnsetsDS struct and its associated memory

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