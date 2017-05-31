#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "onsetStrat.h"

OnsetStrategyFunc chooseOnsetStrategy(char* name)
{
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

float* OnsetsDSDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			    int hpsOvr, int fftSize, int samplerate)
{
	float* tmp;
	return tmp;
}