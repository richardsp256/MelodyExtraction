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
		fflush(NULL);
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
	if((*noteRanges) == NULL){
		printf("malloc error\n");
		fflush(NULL);
		return -1;
	}

	for (i_aR=0; i_aR<numRanges; i_aR++){

		range_start = activityRanges[i_aR * 2];
		range_end = activityRanges[i_aR * 2 + 1];
		
		for (i_onsets = i_onsets; i_onsets<onset_size;i_onsets++){
		
			if (onsets[i_onsets] <= range_start){
				continue;
			} else if (onsets[i_onsets] >= range_end){
				break;
			} else {
				//only add note if it isn't too close to the beginning or end of another note
				if((onsets[i_onsets] - range_start) > threshold
					&& (range_end - onsets[i_onsets]) > threshold){
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

	if(j == 0){ //if no onsets found, do not realloc here. Unable to realloc array to size 0
		return j;
	}

	int *temp;
	temp = realloc((*noteRanges), j*sizeof(int));
	if (temp!= NULL){
		(*noteRanges) = temp;
	} else {
		printf("Resizing noteRanges failed. Exitting.\n");
		free(*noteRanges);
		(*noteRanges) = NULL; //when returning in error, calling func checks if (*noteranges) is NULL, if not, it needs to be freed
		return -1;
	}
	return j;
}

int assignNotePitches(float* freq, int length, int* noteRanges, int nR_size,
		      int winInt, int winSize, int numSamples,
		      float** noteFreq){
	// in the future, we may want to do some kind of convolution of
	// noteFreq to see if there are multiple slurred notes not detected
	// in onset detection
	
	if(nR_size == 0){ //if no note ranges found, no note frequencies found
		(*noteFreq) = malloc(sizeof(float)*1); //we still malloc noteFreq bc calling function expects it
		return 0;
	}

	int i;
	int nF_size = nR_size/2;
	(*noteFreq) = malloc(sizeof(float)*nF_size);
	for (i=0; i<nF_size;i++){
		(*noteFreq)[i] = averageFreq(noteRanges[2*i],
						noteRanges[2*i+1],
						winInt, winSize, numSamples,
						freq,length);
		printf("  Freq: %f \n",(*noteFreq)[i]);
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

	//printf("   startsample %d, stopsample %d\n", startSample, stopSample);
	//printf("   startind %d, stopind %d\n", start_index, stop_index);

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
	//printf("    temp init %f with num_samples %d and freq %f\n", temp, n, freq[start_index]);
	
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
		//printf("    temp update %f with num_samples %d and freq %f\n", temp, num_samples, freq[i]);
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
		//printf("    temp update %f with num_samples %d and freq %f\n", temp, num_samples, freq[stop_index]);
	}

	//printf("   temp  %f,  n  %d\n", temp, n);
	
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
	// there are (bpm/60)*division ticks per second.
	// thus, we return:
	// (bpm*division)/(60*sample_rate) * {s_0, s_1-s_0, ..., s_i-s_(i-1), ...}

	int* eventRanges = malloc(sizeof(int)*nR_size);

	double ticks_per_sample = (bpm * division)/(sample_rate * 60.0);

	for(int i =0; i<nR_size; i++){
		int delta_samples = noteRanges[i];
		if(i > 0){
			delta_samples -= noteRanges[i-1];
		}
		eventRanges[i] = (int)round(delta_samples * ticks_per_sample);
	}
	return eventRanges;
}


int genNotesFreqOnly(int* midiPitches, int length, int winInt, int winSize,
		     int numSamples, int** noteRanges, int** notePitches)
{
	// this function calculates the start and end points from detected
	// frequencies; it in no way depends on onset or silence detection
	// this function only exists to maintain backwards compatability

	// for now, set the number of ranges to the maximum possible size
	// which is 2 * ceil(numFrames/2) and notePitches to maximum possible
	// size which is ceil(numFrames/2)

	int nR_size, nP_size, i,j,last;
	int* temp;

	if (length%2 == 0){
		nR_size = length;
	} else {
		nR_size = length + 2;
	}

	nP_size = nR_size/2;
	(*noteRanges) = malloc(sizeof(int)*nR_size);
	(*notePitches) = malloc(sizeof(int)*nP_size);

	// let's start with the very first entry
	
	last = 0;
	j = 0;	

	// proceed to the index of the first window with non-zero pitch
	for (i = 0; i<length; i++){
		if (midiPitches[i]>0){
			break;
		}
	}

	for (i =i; i<length;i++){

		if (midiPitches[i]==last){
			continue;
		}
		
		if (last != 0){
			// this means that the previous group of contiguous
			// windows did not have a Pitch of 0 (which means there
			// is silence) or that this is not the very first window
			
			// Thus, we must enter the index of the sample where
			// the previous contiguous group of pitchs stopped
			
			(*noteRanges)[j] = winStopRepSampleIndex(winInt,
								 winSize,
								 numSamples,
								 i-1);
			j++;
		}

		if (midiPitches[i]!=0){
			// this means the group of contiguous windows which are
			// just starting do not have a Pitch of 0 (are silent).
			// Therefore we must enter the index of the sample
			// where the contiguous group of constant pitch begins
			// and the actual pitch
			(*noteRanges)[j] = winStartRepSampleIndex(winInt,
								  winSize,
								  numSamples,
								  i);
			(*notePitches)[j/2] = midiPitches[i];
			j++;
		}
	}

	// finally, let's check the final pitch
	if (j%2 == 1){
		// this means that we need to signal the end of the final note.
		(*noteRanges)[j] = numSamples;
	}

	// before we finish, we must resize notePitches and noteRanges

	nR_size = j;
	nP_size = j/2;
	
	temp = realloc(*noteRanges,sizeof(int)*nR_size);
	if (temp!=NULL){
		*noteRanges = temp;
	} else {
		printf("Resizing noteRanges failed. Exitting.\n");
		free(*noteRanges);
		free(*notePitches);
		return -1;
	}
	
	temp = realloc(*notePitches,sizeof(int)*nP_size);
	if (temp!=NULL){
		*notePitches = temp;
	} else {
		printf("Resizing notePitches failed. Exitting.\n");
		free(*noteRanges);
		free(*notePitches);
		return -1;
	}

	return nP_size;
}
