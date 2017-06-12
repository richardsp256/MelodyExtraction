#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "HPSDetection.h"

float* HarmonicProductSpectrum(double** AudioData, int size, int dftBlocksize, int hpsOvr, int samplerate)
{
	//for now, doesn't attempt to distinguish if a note is or isnt playing.
	//if no note is playing, the dominant tone will just be from the noise.

	int i,j,limit, numBlocks, loudestIndex;
	double loudestOfBlock;
	assert(size % dftBlocksize == 0);
	numBlocks = size / dftBlocksize;

	float* loudestFreq = malloc( sizeof(float) * numBlocks );

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
				loudestIndex = i;
			}
		}
		loudestFreq[blockstart/dftBlocksize] = BinToFreq(loudestIndex, dftBlocksize, samplerate);
	}

	free(AudioDataCopy);

	return loudestFreq;
}

float BinToFreq(int bin, int dftBlocksize, int samplerate){
	return bin * (float)samplerate / dftBlocksize;
} 
