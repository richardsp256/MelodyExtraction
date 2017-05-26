#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "BaNaDetection.h"
#include "detectionStrat.h"

FundamentalDetectionStrategy chooseStrategy(char* name)
{
	// this function returns the fundamental detection strategy named name
	// all names are case insensitive
	// this function returns NULL if the name is invalid

	FundamentalDetectionStrategy detectionStrategy;

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

int* HPSDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			  int hpsOvr, int fftSize, int samplerate)
{
	return HarmonicProductSpectrum(AudioData, size, dftBlocksize, hpsOvr);
}

int* BaNaDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			   int hpsOvr, int fftSize, int samplerate)
{
	double* fundamentals;
	int* bins;
	fundamentals = BaNa(AudioData, size, dftBlocksize, 5, 50, 600,
			    fftSize, samplerate);
	bins = FreqToBin(fundamentals, fftSize, samplerate, size,
			 dftBlocksize);
	free(fundamentals);
	return bins;
}

int* BaNaMusicDetectionStrategy(double** AudioData, int size, int dftBlocksize,
				int hpsOvr, int fftSize, int samplerate)
{
	double* fundamentals;
	int* bins;
	fundamentals = BaNaMusic(AudioData, size, dftBlocksize, 5, 50, 3000,
				 fftSize, samplerate);
	bins = FreqToBin(fundamentals, fftSize, samplerate, size,
			 dftBlocksize);
	free(fundamentals);
	return bins;
}

int* HarmonicProductSpectrum(double** AudioData, int size, int dftBlocksize, int hpsOvr)
{
	//for now, doesn't attempt to distinguish if a note is or isnt playing.
	//if no note is playing, the dominant tone will just be from the noise.

	int i,j,limit, numBlocks;
	double loudestOfBlock;
	assert(size % dftBlocksize == 0);
	numBlocks = size / dftBlocksize;

	int* loudestIndices = malloc( sizeof(int) * numBlocks );

	//create a copy of AudioData
	double* AudioDataCopy = malloc( sizeof(double) * dftBlocksize );
	printf("size: %d\n", size);
	printf("dftblocksize: %d\n", dftBlocksize);

	//do each block at a time.
	for(int blockstart = 0; blockstart < size; blockstart += dftBlocksize){

		//copy the block
		for(i = 0; i < dftBlocksize; ++i){
			AudioDataCopy[i] = (*AudioData)[blockstart + i];
		}

		for(i = 2; i <= hpsOvr; i++){
			limit = dftBlocksize/i;
			for(j = 0; j <= limit; j++){
				(*AudioData)[blockstart + j] *= AudioDataCopy[j*i];
			}
		}

		loudestOfBlock = 0.0;
		for(i = 0; i < dftBlocksize; ++i){
			if((*AudioData)[blockstart + i] > loudestOfBlock){
				loudestOfBlock = (*AudioData)[blockstart + i];
				loudestIndices[blockstart/dftBlocksize] = i;
			}
		}
	}

	free(AudioDataCopy);

	return loudestIndices;
}
