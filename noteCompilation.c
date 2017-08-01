#include <stdio.h>
#include <stdlib.h>
#include "noteCompilation.h"
#include "winSampleConv.h"
#include "math.h"

// Here we lists functions that allow us to determine when musical notes happen
// by compiling the information from Pitch Detection, Onset Detection, and
// Activity Detection


/*
 * creates noteRanges which follows a similar format to activityRanges
 */
int calcNoteRanges(int* onsets, int onset_size, int* activityRanges,
		   int aR_size, int** noteRanges, int samplerate)
{
	if (aR_size == 0){
		printf("No activity was detected. Exitting.\n");
		return -1;
	}

	int numRanges = aR_size/2;
	int i_onsets = 0;
	int j = 0;
	int i_aR, range_end, range_start;
	/*
	If two onsets, or an onset and an activityrange start, 
	are within [threshold] of eachother, we deem them as the same note.
	The threshold I chose below is equivalent to 40ms for the samplerate. 
	A 32nd note at 144bpm (very short) is 52 ms,
	so i think its a safe bet anything less than 40ms is not 2 different notes.
	*/
	int threshold = (int)(40 * samplerate / 1000); 

	(*noteRanges) = malloc(sizeof(int)*(onset_size * 2 + aR_size));

	for (i_aR=0; i_aR<numRanges; i_aR++){

		range_start = activityRanges[i_aR * 2];
		range_end = activityRanges[i_aR * 2 + 1];
		
		for (i_onsets = i_onsets; i_onsets<onset_size;i_onsets++){
		
			if (onsets[i_onsets] <= range_start){
				continue;
			} else if (onsets[i_onsets] >range_end){
				break;
			} else {
				if((onsets[i_onsets] - range_start) > threshold){
					(*noteRanges)[j] = range_start;
					j++;
					(*noteRanges)[j] = onsets[i_onsets];
					j++;
					range_start = onsets[i_onsets];
				}
			}
		}

		(*noteRanges)[j] = range_start;
		j++;
		(*noteRanges)[j] = range_end;
		j++;
	}

	int *temp;
	temp = realloc((*noteRanges), j*sizeof(int));
	if (temp!= NULL){
		(*noteRanges) = temp;
	} else {
		printf("Resizing noteRanges failed. Exitting.\n");
		free(*noteRanges);
	}
	return j;
}

int assignNotePitches(float* freq, int length, int* noteRanges, int nR_size,
		      int winInt, int winSize, int numSamples,
		      float** noteFreq){
	// in the future, we may want to do some kind of convolution of
	// noteFreq to see if there are multiple slurred notes not detected
	// in onset detection

	int i;
	int nF_size = nR_size/2;
	(*noteFreq) = malloc(sizeof(float)*nF_size);
	for (i=0; i<nF_size;i++){
		(*noteFreq)[i] = averageFreq(noteRanges[2*i],
						noteRanges[2*i+1],
						winInt, winSize, numSamples,
						freq,length);
		printf("Freq: %f \n",(*noteFreq)[i]);
	}
	return nF_size;
}

float averageFreq(int startSample, int stopSample, int winInt, int winSize,
		  int numSamples, float* freq, int length)
{
	if (startSample == stopSample){
		return 0;
	}
	// take the weighted average
	int start_index = repWinIndex(winInt, winSize, numSamples, startSample);
	int stop_index = repWinIndex(winInt, winSize, numSamples, stopSample);

	if ((stop_index - start_index) == 0){
		// this is the scenario where the entire note occurs within a
		// single window
		return freq[start_index];
	}
	
	// The first and last windows will have different weighting from the
	// rest of the windows
	// now we will handle the first window
	int n = winStopRepSampleIndex(winInt, winSize, numSamples, start_index)
		- startSample;
	double temp = ((double)n) * ((double)freq[start_index]);
	
	// now we will handle all windows between the first and last window
	// we calculate the standard number of samples per window. This applies 
	// to all windows except for the first and last windows (which have
	// indices of 0 and length -1) and the windows where the note starts
	// and stops
	int standard_size = winStopRepSampleIndex(winInt, winSize, numSamples,
						  start_index+1)
		- winStartRepSampleIndex(winInt, winSize, numSamples,
					 start_index+1);
	
	int num_samples;
	// if start_index+1 is the very last window, then it is inconsequential
	for (int i = start_index; i<stop_index; i++){
		if (i == length-1){
			num_samples = winStopRepSampleIndex(winInt, winSize,
							    numSamples,
							    i);
			num_samples -= winStartRepSampleIndex(winInt, winSize,
							      numSamples, i);
		} else {
			num_samples = standard_size;
		}
		temp += (double)num_samples*(double)freq[i];
		n+=num_samples;
	}

	// it is possible for the window with index stop_index to have some
	// samples within the note
	int final_window_start = winStartRepSampleIndex(winInt, winSize,
							numSamples,
							stop_index);
	if (final_window_start < stopSample){
		// this is only possible if stop_index <= length-1
		num_samples = stopSample - final_window_start;
		temp += (double)num_samples*(double)freq[stop_index];
		n+=num_samples;
	}
	
	return (float)(temp/(double)n);
}

float medianFreq(int startSample, int stopSample, int winInt, int winSize,
		 int numSamples, float *freq, int length)
{
	// this is not written well - could get much better complexity
	float *temp = malloc(sizeof(float) * (stopSample-startSample));
	int start_index = repWinIndex(winInt, winSize, numSamples, startSample);
	int stop_index = repWinIndex(winInt, winSize, numSamples, stopSample);
	int i;
	int n = stop_index-start_index;
	for (i = 0; i<(n); i++){
		temp[i] = freq[i+start_index];
	}
	// the following for loop comes from:
	// https://en.wikiversity.org/wiki/C_Source_Code/Find_the_median_and_mean
	int j;
	float temp_freq;

	for(i=0; i<n-1; i++) {
		for(j=i+1; j<n; j++) {
			if(freq[j] < freq[i]){
				// swap elements
				temp_freq = temp[i];
				temp[i] = temp[j];
				temp[j] = temp_freq;
			}
		}
	}
	if (n%2==0){
		temp_freq = (temp[n/2]+temp[n/2-1])/2.0;
	} else {
		temp_freq = temp[n/2];
	}
	free(temp);
	return temp_freq;
}

int* noteRangesEventTiming(int* noteRanges, int nR_size, int sample_rate,
			   int bpm, int division)
{
	// convert samples in noteRanges to midi event timings
	// noteRanges is an array of the number of samples from the
	// beginning that dictate when notes start and stop using the
	// sample rate of the input audio file:
	// {s_0, s_1, s_2, ..., s_i, ... }

	// the returned eventRanges lists the amount of ticks from the
	// previous entry. Thus our first step is to determine the sample
	// from previous entry:
	// {s_0, s_1-s_0, s_2 - s_1, ..., s_i - s_(i-1), ...}

	// then we convert these entries so that they have units of seconds
	// (1/sample_rate) * {s_0, s_1-s_0, s_2 - s_1, ..., s_i - s_(i-1), ...}

	// finally, we need to convert into units of ticks. In the midi file
	// there are (bpm/(60*division)) ticks per second.
	// thus, we return:
	// bpm/(60*sample_rate*division) * {s_0, s_1-s_0, ..., s_i-s_(i-1), ...}

	int* eventRanges = malloc(sizeof(int)*nR_size);

	eventRanges[0] = (int)round(((double)bpm/(60.* (double)sample_rate
						  * (double)division)
				     * (double)noteRanges[0]));
	for(int i =1; i<nR_size; i++){
		eventRanges[i] = (int)round(((double)bpm/(60.
							  * (double)sample_rate
							  * (double)division)
					     * (double)(noteRanges[i]
							- noteRanges[i-1])));
	}
	return eventRanges;
}