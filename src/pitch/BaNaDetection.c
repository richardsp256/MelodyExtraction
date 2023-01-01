#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "findpeaks.h"
#include "findCandidates.h"
#include "candidateSelection.h"
#include "../lists.h"
#include "../errors.h"
#include "BaNaDetection.h"

// MAX_P defines the maximum number of harmonic peaks. 10 is definitely overkill
#define MAX_P 10

// here we define the actual BaNa Fundamental pitch detection algorithm
// NOTE: in all of my code I use window, frame, and block to refer to the same
//       thing

int BaNa(float *spectrogram, int size, int dftBlocksize, int p,
	 float f0Min, float f0Max, float xi, int fftSize, int samplerate,
	 bool first, float *fundamentals)
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

	float *frequencies = calcFrequencies(dftBlocksize, fftSize,
					      samplerate);
	if(frequencies == NULL){
		return ME_MALLOC_FAILURE;
	}

	// preprocess each of the frames
	// In the future, we will try to avoid modifying the input spectrogram
	// directly.
	BaNaPreprocessing(spectrogram, size, dftBlocksize, p, f0Min, f0Max,
			  frequencies);

	int numBlocks = (size / dftBlocksize);
	distinctList **windowCandidates =
		malloc(sizeof(distinctList*) * numBlocks);
	if(windowCandidates == NULL){
		free(frequencies);
		return ME_MALLOC_FAILURE;
	}

	// set smoothwidth to the equivalent of 50 Hz
	// (not totally sure I should be using fftSize instead of dftBlocksize)
	float smoothwidth = 50. * ( ((float) fftSize) /
				    ((float) samplerate) );

	// find the candidates for the fundamentals
	int rv = BaNaFindCandidates(spectrogram, size, dftBlocksize,
				    p, f0Min, f0Max, first, xi, frequencies,
				    smoothwidth, windowCandidates);
	if (rv != ME_SUCCESS){
		free(windowCandidates);
		free(frequencies);
		return rv;
	}

	// determine which candidate is the fundamental
	candidateSelection(windowCandidates, numBlocks, fundamentals);

	// clean up
	for (size_t i=0;i<numBlocks;i++){
		distinctListDestroy(windowCandidates[i]);
	}
	free(windowCandidates);
	free(frequencies);

	return ME_SUCCESS;
}

float* calcFrequencies(int dftBlocksize, int fftSize, int samplerate)
{
	// returns an array of length dftBlocksize with the frequency
	// value for every bin
	int i;
	float* frequencies = malloc(dftBlocksize * sizeof(float));
	float ratio = ((float)samplerate) / ((float)fftSize);
	if(frequencies != NULL){
		for (i=0;i<dftBlocksize;i++) {
			frequencies[i] = i*ratio;
		}
	}
	return frequencies;
}

void BaNaPreprocessing(float *spectrogram, int size, int dftBlocksize, int p,
		       float f0Min, float f0Max, const float* frequencies)
{
	// set all frequencies outside of [f0Min, p * f0Max] to zero
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
	for (int blockstart=0; blockstart < size; blockstart += dftBlocksize){
		for (int i=0; i<goodFreqStart; i++){
			spectrogram[blockstart + i] = 0;
		}
		for (int i=goodFreqStop; i<dftBlocksize; i++){
			spectrogram[blockstart + i] = 0;
		}
	}
}

int BaNaFindCandidates(const float *spectrogram, int size,
		       int dftBlocksize, int p, float f0Min,
		       float f0Max, bool first, float xi,
		       float* frequencies, float smoothwidth,
		       distinctList** windowCandidates)
{
	// this finds all of the f0 candiates
	// windowCandidates is an array of pointers that point to the list of
	// candidates for each window

	float peakFreq[MAX_P];
	float peakMag[MAX_P];
	if (p > MAX_P){
		return ME_BANA_TOO_MANY_PEAKS;
	}

	// outer loop iterates over blocks
	for (int win =0; win < size/dftBlocksize; win++ ){

		const float * const magnitudes =
			&(spectrogram[win*dftBlocksize]);

		// determine slopeThreshold, ampThreshold, smoothwidth
		// set ampThreshold to 1/15 th of largest magnitude
		float ampThreshold = magnitudes[0];
		for (int i=1; i < dftBlocksize; i++){
			float temp = magnitudes[i];
			if (temp>ampThreshold) {
				ampThreshold = temp;
			}
		}
		ampThreshold/=15.;

		// find the harmonic spectra peaks
		float firstFreqPeak;
		int numPeaks = findpeaks(frequencies, magnitudes,
					 (long)dftBlocksize, 0.0,
					 ampThreshold, smoothwidth, 5, 3, 
					 p, first, peakFreq, peakMag,
					 &firstFreqPeak);

		// Now, identify the candidates

		// Max number of candidates from ratio analysis
		//      = (numPeaks)*(numPeaks-1)/2
		// Need additional space to hold:
		//   - lowest frequency peak
		//   - cepstral frequency
		struct orderedList candidates =
			orderedListCreate( (numPeaks * (numPeaks-1))/2 + 2);

		// determine the candidates from the peak ratios
		RatioAnalysisCandidates(peakFreq, numPeaks, &candidates);

		// add the lowest frequency peak fundamental candidate
		orderedListInsert(&candidates, (float)firstFreqPeak);

		// add the cepstrum fundamental candidate
		// (skipping this for now!)

		// Now consolidate the list of all candidates into a
		// distinctive list.
		distinctList* t = distinctCandidates(&candidates,
						     ((numPeaks)*
						     (numPeaks-1)/2)+2,
						     xi,(float)f0Min,
						     (float)f0Max);
		orderedListDestroy(candidates);
		if (t == NULL){
			for(int i=0; i <win; i++){
				distinctListDestroy(windowCandidates[i]);
			}
			return ME_MALLOC_FAILURE;
		}

		// Finally, add the distinctive candidates to windowCandidates
		windowCandidates[win] = t;
	}
	return ME_SUCCESS;
}
