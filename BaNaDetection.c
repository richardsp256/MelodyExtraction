#include <stdlib.h>
#include <stdio.h>
#include "findpeaks.h"
#include "findCandidates.h"
#include "candidateSelection.h"
#include "lists.h"
#include "BaNaDetection.h"


// here we define the actual BaNa Fundamental pitch detection algorithm
// NOTE: in all of my code I use window, frame, and block to refer to the same
//       thing

// NOTE: modify use of doubles to represent frequency to floats for consistency

float* BaNa(float **AudioData, int size, int dftBlocksize, int p,
	    float f0Min, float f0Max, float xi, int fftSize, int samplerate,
	    int first)
{
	// Implements the BaNa fundamental pitch detection algorithm
	// This algorithm is split into 3 parts:

	// 1. Preprocessing
	//    - mask all frequencies outside of [f0Min, p * f0Max]
	// 2. Determination of F0 candidates. For each frame:
	//    a. Retrieve p harmonic peaks - for ordinary BaNa algorithm, these
	//       are the p peaks with lowest frequencies
	//    b. Calculate F0 candidates from harmonic peaks
	//    c. Add F0 found by Cepstrum to the F0 candidates
	//    d. Add harmonic peak with lowest frequency to F0 candidates
	// 3. Post-Processing. Selection of F0 from candidates


	float *fundamentals;
	struct distinctList **windowCandidates;
	int numBlocks = size / dftBlocksize;
	long i;

	float *frequencies = calcFrequencies(dftBlocksize, fftSize,
					      samplerate);
	
	// preprocess each of the frames
	BaNaPreprocessing(AudioData, size, dftBlocksize, p, f0Min, f0Max,
			  frequencies);

	// find the candidates for the fundamentals
	windowCandidates = BaNaFindCandidates(AudioData, size, dftBlocksize,
					      p, f0Min, f0Max, first, xi,
					      frequencies, fftSize, samplerate);

	// determine which candidate is the fundamental
	fundamentals = candidateSelection(windowCandidates, numBlocks);
	
	// clean up
	for (i=0;i<numBlocks;i++){
		distinctListDestroy(windowCandidates[i]);
	}
  
	free(windowCandidates);
	free(frequencies);
	return fundamentals;
}

float* calcFrequencies(int dftBlocksize, int fftSize, int samplerate)
{
	// returns an array of length dftBlocksize with the frequency
	// value for every bin
	int i;
	float* frequencies = malloc(dftBlocksize * sizeof(float));
	float ratio = ((float)samplerate) / ((float)fftSize);
	
	for (i=0;i<dftBlocksize;i++) {
		frequencies[i] = i*ratio;
	}
	return frequencies;
}

void BaNaPreprocessing(float **AudioData, int size, int dftBlocksize, int p,
		       float f0Min, float f0Max, float* frequencies)
{
	// set all frequencies outside of [f0Min, p * f0Max] to zero
	int blockstart, i;
	float maxFreq = (((float)p)*f0Max);
	// determine the index of the leftmost frequency value >= f0Min
	int goodFreqStart = bisectLeft(frequencies, f0Min, 0, dftBlocksize);
	// determine the index of the leftmost frequency value > maxFreq
	int goodFreqStop = bisectLeft(frequencies, maxFreq, goodFreqStart,
				       dftBlocksize);
	if (goodFreqStop != dftBlocksize) {
		if (frequencies[goodFreqStop] == maxFreq){
			goodFreqStop++;
		}
	}

	// set magnitudes for frequencies outside of [f0Min,maxFreq] to 0
	for (blockstart = 0; blockstart < size; blockstart += dftBlocksize){
		for (i=0; i<goodFreqStart; i++){
			(*AudioData)[blockstart + i] = 0;
		}
		for (i=goodFreqStop; i<dftBlocksize; i++){
			(*AudioData)[blockstart + i] = 0;
		}
	}
}

struct distinctList** BaNaFindCandidates(float **AudioData, int size,
					 int dftBlocksize, int p,
					 float f0Min, float f0Max,
					 int first, float xi,
					 float* frequencies, int fftSize,
					 int samplerate)
{
	// this finds all of the f0 candiates
	// windowCandidates is an array of pointers that point to the list of
	// candidates for each window


	int i;
	int numBlocks = size / dftBlocksize;
	int blockstart;
	int numPeaks;
	float *magnitudes = malloc(dftBlocksize * sizeof(float));
	float temp, firstFreqPeak, ampThreshold, smoothwidth;
	float *peakFreq, *peakMag;

	peakFreq = malloc(p * sizeof(float));
	peakMag = malloc(p * sizeof(float));

	struct orderedList candidates;

	struct distinctList **windowCandidates;
	windowCandidates = malloc(sizeof(struct distinctList*) * numBlocks);

	// outer loop iterates over blocks
	for (blockstart = 0; blockstart < size; blockstart += dftBlocksize){

		// for now copy the magnitudes into a buffer. Fix this in the
		// future
		for(i = 0; i < dftBlocksize; i++){
			magnitudes[i] = (*AudioData)[blockstart + i];
		}

		// determine slopeThreshold, ampThreshold, smoothwidth
		// set ampThreshold to 1/15 th of largest magnitude
		ampThreshold = magnitudes[0];
		for (i=1; i < dftBlocksize; i++){
			temp = magnitudes[i];
			if (temp>ampThreshold) {
				ampThreshold = temp;
			}
		}
		ampThreshold/=15.;

		// set smoothwidth to the equivalent of 50 Hz
		smoothwidth = 50. * ((float) fftSize) / ((float) samplerate);

		// find the harmonic spectra peaks
		numPeaks = findpeaks(frequencies, magnitudes,
				     (long)dftBlocksize, 0.0,
				     ampThreshold, smoothwidth, 5, 3, 
				     p, first, peakFreq, peakMag,
				     &firstFreqPeak);
		//printf("Candidates = [%.0lf",firstFreqPeak);
		//for (int j =1; j<numPeaks;j++){
		//	printf(", %.0lf",peakFreq[j]);
		//}
		//printf("]\n");
		
		//printf("Done finding peaks\n");
		// determine the candidates from the peaks
		candidates = calcCandidates(peakFreq, numPeaks);
		//printf("Done finding candidates\n");
		// add the lowest frequency peak fundamental candidate
		orderedListInsert(&candidates, (float)firstFreqPeak);
		//printf("Inserted lowest frequency candidate\n");
		// add the cepstrum fundamental candidate

		struct distinctList* t = distinctCandidates(&candidates,
							     (p-1)*(p-1)+2,
							     xi,(float)f0Min,
							     (float)f0Max);
		//distinctListPrintFreq(*t);
		// determine the distinctive candidates and add them to
		windowCandidates[blockstart/dftBlocksize] = t;
		//windowCandidates[i] = distinctCandidates(&candidates,
		//					 (p-1)*(p-1)+2, xi);

		orderedListDestroy(candidates);

		 
	}
	free(peakFreq);
	free(peakMag);
	free(magnitudes);
	return windowCandidates;
}


/*
int main(int argc, char*argv[])
{
	// only including this for testing purposes
	return 0;
}
*/
