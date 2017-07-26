// Utility functions used to determine which samples are associated with which
// windows

#include <stdio.h>
#include <stdlib.h>
#include "winSampleConv.h"

int numWindows(int winInt, int winSize, int numSamples){
	// calculates the number of samples
	if (numSamples<winSize){
		return 1;
	} else {
		return (numSamples-winSize-1)/winInt +2;
	}
}

int winStartSampleIndex(int winInt, int winInd){
	// calculates the index of the first sample used in calculations
	// involving the window with index winInd
	return winInd*winInt;
}

int winStartRepSampleIndex(int winInt, int winSize, int numSamples, int winInd){
	// calculates the index of the first sample that the window, with index
	// winInd, is representative of.
	if (winInd == 0){
		return 0;
	} else {
		// first lets calculate the first sample in the window
		int start_index = winStartSampleIndex(winInt, winInd);

		start_index += ((winSize-winInt)/2);
		// the following if statement might be unnecessary
		if (start_index<numSamples){
			start_index++;
		}
		return start_index;
	}
}

int winStopSampleIndex(int winInt, int numSamples,int winInd){
	// returns the index of the sample following the final sample used in
	// calculations involving the window with index winInd
	int out = winStartSampleIndex(winInt, winInd+1);
	if (out > numSamples){
		return numSamples;
	} else {
		return out;
	}
}

int winStopRepSampleIndex(int winInt, int winSize, int numSamples, int winInd){
	// returns the index of the sample following the final sample that the
	// window, at index winInd, is representative of
	int out = winStartRepSampleIndex(winInt, winSize, numSamples, winInd+1);
	if (out > numSamples){
		return numSamples;
	} else {
		return out;
	}
}

int repWinIndex(int winInt, int winSize, int numSamples, int sampleIndex){
	// determines the index of the window that is representive of the
	// sample with index SampleIndex
	// this function assumes sampleIndex<numSamples

	// calculate the index of the sample following the last sample in
	// the first window that the first window is representative of simply
	// because there are no preceeding windows
	int offset = ((winSize-winInt)/2);
	if (sampleIndex < offset){
		return 0;
	} else {
		// first let's subtract off the number of samples that the
		// first window is representative of simply because there are
		// no preceeding windows, divide by the window interval and add
		// one
		int winIndex = ((sampleIndex - offset)/winInt);
		// finally check if the index is greater than the
		// index of the last window. In that case, we return the index
		// of the last window

		int maxIndex = numWindows(winInt, winSize, numSamples)-1;
		if (winIndex > maxIndex ){
			return maxIndex;
		}
		return winIndex;
			
	}
}
