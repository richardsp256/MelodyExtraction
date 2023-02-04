#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include "HPSDetection.h"

int HarmonicProductSpectrum(float** AudioData, int size, int dftBlocksize, int hpsOvr, int fftSize, int samplerate, float*loudestFreq)
{
	//for now, doesn't attempt to distinguish if a note is or isnt playing.
	//if no note is playing, the dominant tone will just be from the noise.

	int i,j,limit, numBlocks;
	float loudestOfBlock;
	int loudestIndex = -1;
	assert(size % dftBlocksize == 0);

	// numBlocks is the length of loudestFreq
	numBlocks = size / dftBlocksize;


	//create a copy of AudioData
	float* AudioDataCopy = malloc( sizeof(float) * dftBlocksize );
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

		loudestOfBlock = FLT_MIN;
		for(i = 0; i < dftBlocksize; ++i){
			if((*AudioData)[blockstart + i] > loudestOfBlock){
				loudestOfBlock = (*AudioData)[blockstart + i];
				loudestIndex = i;
			}
		}
		loudestFreq[blockstart/dftBlocksize] = BinToFreq(loudestIndex, fftSize, samplerate);
	}

	free(AudioDataCopy);

	return 1;
}

float BinToFreq(int bin, int fftSize, int samplerate){
	return bin * (float)samplerate / fftSize;
} 
