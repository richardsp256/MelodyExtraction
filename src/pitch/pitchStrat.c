#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "BaNaDetection.h"
#include "HPSDetection.h"
#include "pitchStrat.h"

PitchStrategyFunc choosePitchStrategy(char* name)
{
	// this function returns the fundamental detection strategy named name
	// all names are case insensitive
	// this function returns NULL if the name is invalid

	PitchStrategyFunc detectionStrategy;

	// Lets make name lowercased
	// https://stackoverflow.com/questions/2661766/c-convert-a-mixed-case-
	//string-to-all-lower-case

	for(int i = 0; name[i]; i++){
		name[i] = tolower(name[i]);
	}
	
	if (strcmp(name,"hps")==0) {
		detectionStrategy = &HPSDetectionStrategy;
	} else if (strcmp(name,"bana")==0) {
		detectionStrategy = &BaNaDetectionStrategy;
	} else if (strcmp(name,"banamusic")==0) {
		detectionStrategy = &BaNaMusicDetectionStrategy;
	} else {
		detectionStrategy = NULL;
	}
	return detectionStrategy;
}

int HPSDetectionStrategy(float* spectrogram, int size, int dftBlocksize,
			 int hpsOvr, int fftSize, int samplerate,
			 float *pitches)
{
	return HarmonicProductSpectrum(spectrogram, size, dftBlocksize, hpsOvr,
				       fftSize, samplerate, pitches);
}

int BaNaDetectionStrategy(float* spectrogram, int size, int dftBlocksize,
			  int hpsOvr, int fftSize, int samplerate,
			  float *pitches)
{
	return BaNa(spectrogram, size, dftBlocksize, 5, 50, 600, 10.0,
		    fftSize, samplerate, true, pitches);
}

int BaNaMusicDetectionStrategy(float* spectrogram, int size,
			       int dftBlocksize, int hpsOvr, int fftSize,
			       int samplerate, float *pitches)
{
	return BaNa(spectrogram, size, dftBlocksize, 5, 50, 3000, 3.0,
		    fftSize, samplerate, false, pitches);
}
