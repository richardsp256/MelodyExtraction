#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include "HPSDetection.h"

int HarmonicProductSpectrum(const float* AudioData, int size, int dftBlocksize, int hpsOvr, int fftSize, int samplerate, float*loudestFreq)
{
	//for now, doesn't attempt to distinguish if a note is or isnt playing.
	//if no note is playing, the dominant tone will just be from the noise.
	assert(size % dftBlocksize == 0);

	// create a copy of HPSpectrum, which should be large enough to hold
	// one of the spectra from AudioData
	float* HPSpectrum = malloc( sizeof(float) * dftBlocksize );
	printf("size: %d\n", size);
	printf("dftblocksize: %d\n", dftBlocksize);

	//do each block at a time.
	for(int blockstart = 0; blockstart < size; blockstart += dftBlocksize){

	       // set all of the values held by HPSpectrum to 1
		for(int i = 0; i < dftBlocksize; ++i){
			HPSpectrum[i] = 1.f;
		}

		for(int n = 1; n <= hpsOvr; n++){
			// Compute the bin of the frequency above which the nth
			// harmonic is not included in the input spectrum
			int limit = dftBlocksize/n;
			for (int i = 0; i <= limit; i++){
				HPSpectrum[i] *= AudioData[blockstart + i*n];
			}
		}

		float loudestOfBlock = -FLT_MAX;
		int loudestIndex = -1;
		for(int i = 0; i < dftBlocksize; ++i){
			if(HPSpectrum[i] > loudestOfBlock){
				loudestOfBlock = HPSpectrum[i];
				loudestIndex = i;
			}
		}
		loudestFreq[blockstart/dftBlocksize] = BinToFreq(loudestIndex, fftSize, samplerate);
	}

	free(HPSpectrum);

	return 1;
}

float BinToFreq(int bin, int fftSize, int samplerate){
	return bin * (float)samplerate / fftSize;
} 
