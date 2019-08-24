#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "fftw3.h"
#include "comparison.h"
#include "midi.h"
#include "pitchStrat.h"
#include "onsetStrat.h"
#include "silenceStrat.h"
#include "winSampleConv.h"
#include "noteCompilation.h"
#include "tuningAdjustment.h"

float* WindowFunction(int size)
{
	//uses Hamming Window
	float* buffer = malloc(sizeof(float) * size);
	if (buffer != NULL){ //
		for(int i = 0; i < size; ++i) {
			buffer[i] =(float) (0.54-(0.46*cos(2*M_PI*i/(size-1.0))));
		}
	}
	return buffer;
}

struct Midi* ExtractMelody(float** input, audioInfo info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int o_unpaddedSize, int o_winSize, int o_winInt, OnsetStrategyFunc onsetStrategy,
		int s_winSize, int s_winInt, int s_mode, SilenceStrategyFunc silenceStrategy,
		int hpsOvr, int tuning, int verbose, char* prefix)
{

	if(verbose){
		printf("ARGS:\n");
		printf("p_unpad %d,  p_win %d,  p_int %d\n", p_unpaddedSize, p_winSize, p_winInt);
		printf("o_unpad %d,  o_win %d,  o_int %d\n", o_unpaddedSize, o_winSize, o_winInt);
		printf("s_win %d,  s_int %d,  s_mode %d\n", s_winSize, s_winInt, s_mode);
		printf("hps %d,  tuning %d,  verbose %d,  prefix %s\n", hpsOvr, tuning, verbose, prefix);
	}

	int *activityRanges = NULL;
	int a_size = ExtractSilence(input, &activityRanges, info, s_winSize,
				    s_winInt, s_mode, silenceStrategy);
	if(a_size == -1){
		printf("Silence detection failed\n");
		fflush(NULL);
		return NULL;
	}
	if(verbose){
		printf("Silence detection complete\n");
		fflush(NULL);
	}

	float* freq = NULL;
	int freqSize = ExtractPitchAndAllocate(input, &freq, info,
					       p_unpaddedSize, p_winSize,
					       p_winInt, pitchStrategy,
					       hpsOvr, verbose, prefix);
	if(freqSize <=0 ){
		printf("Pitch detection failed\n");
		fflush(NULL);
		free(activityRanges);
		return NULL;
	}
	if(verbose){
		printf("Pitch detection complete\n");
		for(int y = 0; y < freqSize; y++){ 
			printf("  %f\n", freq[y]); 
		} 
		fflush(NULL);
	}

	// Make onsets an intList
	//    - initial size is 20 (might want something different
	//    - max_capacity is set to the default. We could probably be
	//      somewhat intelligent about the max_capacity (but it would
	//      depend on onset strategy)
	intList* onsets = intListCreate(20, 0);


	// Commenting the following out was part of a quick and dirty solution
	// to call the TransientDetectionStrategy. To call this strategy, more
	// correctly (via ExtractOnset), will require some refactoring of the
	// ExtractOnset function. We need to make the onsetStrategy take it's
	// own Short Time Fourier Transform, if necessary. Also the
	// onsetStrategy should probably indicate whether or not offsets are in
	// the output
	//int o_size = ExtractOnset(input, &onsets, info, o_unpaddedSize, o_winSize, o_winInt, 
	//             onsetStrategy, verbose);

	int o_size = TransientDetectionStrategy(input, info.frames, 0,
						info.samplerate, onsets);
	if(o_size == -1){
		printf("Onset detection failed\n");
		fflush(NULL);
		free(activityRanges);
		free(freq);
		intListDestroy(onsets);
		return NULL;
	}
	if(verbose){
		printf("Onset detection complete\n");
		fflush(NULL);
	}


	int *noteRanges = NULL;
	float *noteFreq = NULL;
	int num_notes = ConstructNotes(&noteRanges, &noteFreq, freq,
				     freqSize, onsets, o_size, activityRanges,
				     a_size, info, p_unpaddedSize, p_winInt);

	free(activityRanges);
	free(freq);
	intListDestroy(onsets);

	if(num_notes == -1){
		printf("Construct notes failed!\n");
		fflush(NULL);
		return NULL;
	}
	else if(num_notes == 0){
		printf("No notes detected.\n");
		fflush(NULL);
		return NULL;
	}
	printf("construct notes\n");

	int* melodyMidi = malloc(sizeof(int) * num_notes);
	if(melodyMidi == NULL){
		printf("malloc failed\n");
		fflush(NULL);
		free(noteRanges);
		free(noteFreq);
		return NULL;
	}
	int tmp = FrequenciesToNotes(noteFreq, num_notes, &melodyMidi, tuning);
	if(tmp == -1){
		printf("freqToNote failed\n");
		fflush(NULL);
		free(noteRanges);
		free(noteFreq);
		free(melodyMidi);
		return NULL;
	}

	if (prefix !=NULL){
		// Here we save the note data
		char *noteFile = malloc(sizeof(char) * (strlen(prefix)+11));
		strcpy(noteFile,prefix);
		strcat(noteFile,"_notes.txt");
		SaveNotesTxt(noteFile, noteRanges, melodyMidi, num_notes,
			     info.samplerate);
		free(noteFile);
	}

	char* noteName = calloc(5, sizeof(char));
	printf("Detected %d Notes:\n", num_notes);
	for(int i =0; i<num_notes; i++){
		NoteToName(melodyMidi[i], &noteName);
		printf("%d - %d,   ", noteRanges[2*i], noteRanges[2*i+1]);
		printf("%d ms - %d ms,   ", 
			(int)(noteRanges[2*i] * (1000.0/info.samplerate)),
		    (int)(noteRanges[2*i+1] * (1000.0/info.samplerate)));
		printf("%.2f hz,   ", noteFreq[i]);
		printf("%.2f,   ", FrequencyToFractionalNote(noteFreq[i]));
		printf("%d,   ", melodyMidi[i]);
		printf("%s\n", noteName);
	}

	free(noteName);
	free(noteFreq);
	
	//get midi note values of pitch in each bin

	printf("printout complete\n");
	fflush(NULL);

	struct Midi* midi = GenerateMIDIFromNotes(melodyMidi, noteRanges,
				     num_notes, info.samplerate, verbose);

	free(noteRanges);
	free(melodyMidi);

	if(midi == NULL){
		printf("Midi generation failed\n");
		fflush(NULL);
		return NULL;
	}

	return midi;
}

// allocates memory for pitches and the computes the value of pitches
int ExtractPitchAndAllocate(float** input, float** pitches, audioInfo info,
			    int p_unpaddedSize, int p_winSize, int p_winInt,
			    PitchStrategyFunc pitchStrategy,
			    int hpsOvr, int verbose, char* prefix)
{
	(*pitches) = malloc(sizeof(float) * NumSTFTBlocks(info, p_unpaddedSize,
							  p_winInt));
	if (*pitches == NULL){
		return -1;
	}
	return ExtractPitch(*input, *pitches, info, p_unpaddedSize, p_winSize,
			    p_winInt, pitchStrategy, hpsOvr, verbose, prefix);
}

int ExtractPitch(float* input, float* pitches, audioInfo info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int hpsOvr, int verbose, char* prefix)
{
	fftwf_complex* p_fftData = NULL;
	int p_size = STFT_r2c(&input, info, p_unpaddedSize, p_winSize, p_winInt, &p_fftData);
	if(p_size == -1){
		return -1;
	}
	int p_numBlocks = NumSTFTBlocks(info, p_unpaddedSize, p_winInt);
	if(verbose){
		printf("numblcks of pitch FFT: %d\n", p_numBlocks);
		fflush(NULL);
	}

	float* spectrum = Magnitude(p_fftData, p_size);
	if(spectrum == NULL){
		printf("Magnitude failed\n");
		fflush(NULL);
		free(p_fftData);
		return -1;
	}
	if(verbose){
		printf("Magnitude complete\n");
		fflush(NULL);
	}

	//double* output = NULL;
	//STFTinverse(&p_fftData, info, winSize, p_winInt, &output);
	//if(verbose){
	//	printf("STFTInverse complete\n");
	//}
	free(p_fftData);
	//SaveAsWav(output, info, stftInverse.wav);
	//free( output );

	if (prefix !=NULL){
		// Here we save the original spectra
		char *spectraFile = malloc(sizeof(char) * (strlen(prefix)+14));
		strcpy(spectraFile,prefix);
		strcat(spectraFile,"_original.txt");
		SaveWeightsTxt(spectraFile, &spectrum, p_size, p_winSize/2, info.samplerate, p_unpaddedSize, p_winSize);
		free(spectraFile);
	}

	int result = pitchStrategy(spectrum, p_size, p_winSize/2, hpsOvr,
				   p_winSize, info.samplerate, pitches);
	if(result <= 0){
		return result;
	}

	if (prefix !=NULL){
		// Here we save the processed spectra
		char *spectraFile = malloc(sizeof(char) * (strlen(prefix)+14));
		strcpy(spectraFile,prefix);
		strcat(spectraFile,"_weighted.txt");
		SaveWeightsTxt(spectraFile, &spectrum, p_size, p_winSize/2, info.samplerate, p_unpaddedSize, p_winSize);
		free(spectraFile);
	}

	return p_numBlocks;
}

int ExtractSilence(float** input, int** activityRanges, audioInfo info,
		   int s_winSize, int s_winInt, int s_mode,
		   SilenceStrategyFunc silenceStrategy){
	int a_size = silenceStrategy(input, info.frames, s_winSize, s_winInt,
				     info.samplerate, s_mode,
				     activityRanges);

	if(a_size != -1){ //if exited in error, dont print results
		for (int i = 0; i < a_size; i++){
			printf("Activity Range: %d up to %d\n", (*activityRanges)[i],
			       (*activityRanges)[i+1]);
			i++;
		}
		if(a_size == 0){
			printf("No Activity Ranges found\n");
		}
	}

	return a_size;
}

int ExtractOnset(float** input, intList* onsets, audioInfo info,
		 int o_unpaddedSize, int o_winSize, int o_winInt,
		 OnsetStrategyFunc onsetStrategy, int verbose)
{
	fftwf_complex* o_fftData = NULL;
	int o_fftData_size = STFT_r2c(input, info, o_unpaddedSize, o_winSize,
				      o_winInt, &o_fftData);
	if(o_fftData_size == -1){
		return -1;
	}
	int o_numBlocks = o_fftData_size/(o_winSize/2);
	if(verbose){
		printf("numblcks of onset FFT: %d\n", o_numBlocks);
		fflush(NULL);
	}
	float* o_fftData_float = (float*)o_fftData;
	// because we convert from complex* to float*, o_fftData has twice as
	// many elements
	o_fftData_size *= 2; 

	int o_size = onsetStrategy(&o_fftData_float, o_fftData_size,
				   o_winSize, info.samplerate, onsets);
	//printf("o_strat return size: %d\n", o_size);
	free(o_fftData);

	if(o_size == -1){
		return -1;
	}

	// convert onsets so that the value is not which block of o_fftData it
	// occurred, but which sample of the original audio it occurred
	int* o_arr = onsets->array;
	for (int i = 0; i < onsets->length; i++){
		o_arr[i] = winStartRepSampleIndex(o_winInt, o_unpaddedSize,
						  info.frames, o_arr[i]);
		printf("onset at sample: %d, time: %d, and block: %d\n",
		       o_arr[i], (int)(o_arr[i] * (1000.0/info.samplerate)),
		       repWinIndex(o_winInt, o_unpaddedSize, info.frames,
				   o_arr[i]));
	}

	return o_size;
}

int ConstructNotes(int** noteRanges, float** noteFreq, float* pitches,
		   int p_size, intList* onsets, int onset_size,
		   int* activityRanges, int aR_size, audioInfo info,
		   int p_unpaddedSize, int p_winInt)
{

	// Quick and dirty method to allow us to use the full note ranges
	// returned by TransientDetectionStrategy. In the future, we should
	// extend calcNoteRanges to accept an intList of transients that always
	// includes onsets and optionally includes offsets
	(*noteRanges) = malloc(sizeof(int) * onset_size);
	for(int i = 0; i < onset_size; i++){
		(*noteRanges)[i] = onsets->array[i];
	}
	int nR_size = onset_size;
	
	//int nR_size = calcNoteRanges(onsets, onset_size, activityRanges,
	//			     aR_size, noteRanges, info.samplerate);
	//if(nR_size == -1){
	//	return -1;
	//}

	int nF_size = assignNotePitches(pitches, p_size, *noteRanges, nR_size,
					p_winInt, p_unpaddedSize, info.frames,
					noteFreq);

	//if the pitch for any note is 0 (aka not valid), remove it 
	for(int i = 0; i < nF_size; ++i){ 
		printf("checking pitch %f\n", (*noteFreq)[i]); 
		if((*noteFreq)[i] == 0.0f){ 
			for(int j = i+1; j < nF_size; ++j){
				(*noteFreq)[j-1] = (*noteFreq)[j];
				(*noteRanges)[(j*2)-2] = (*noteRanges)[(j*2)];
				(*noteRanges)[(j*2)-1] = (*noteRanges)[(j*2)+1];
			}
			(*noteFreq) = realloc((*noteFreq), (nF_size -1) * sizeof(float));
			(*noteRanges) = realloc((*noteRanges), (nR_size -2) * sizeof(int));

			nR_size -= 2; 
			nF_size -= 1;

			i--; 
		} 
	}

	return nF_size;
}

int FrequenciesToNotes(float* freq, int num_notes, int**melodyMidi, int tuning)
{
	//it is able to account for singer being sharp/flat
	//in order to get more accurate note estimates
	if(tuning == 0){ //no tuning adjustment
		for(int i = 0; i < num_notes; ++i){
			if(isinf(freq[i])){
				return -1;
			}
			(*melodyMidi)[i] = FrequencyToNote(freq[i]);
		}
		return num_notes;
	}


	float threshold, avg, dist;
	int start, end, neighborSize;

	if(tuning == 1){ //thresholded adjustment
		threshold = 0.0625f;
	}else if(tuning == 2){ //adjust all
		threshold = FLT_MAX;
	}else{
		threshold = 0.0625f;
	}

	float* fractNotes = malloc(sizeof(float) * num_notes);
	for(int i = 0; i < num_notes; ++i){
		fractNotes[i] = FrequencyToFractionalNote(freq[i]);
		if(isinf(fractNotes[i])){
			free(fractNotes);
			return -1;
		}
	}

	for(int i = 0; i < num_notes; ++i){
		//size of neighbors is up to 2 pts to the left + center + up to 2 pts to the right
		start = (int) fmaxf(i-2, 0);
		end = (int) fminf(i+2, num_notes - 1);
		neighborSize = end-start + 1;

		float* neighbors = malloc(sizeof(float) * neighborSize );
		memcpy(&neighbors[0], &fractNotes[start], sizeof(float) * neighborSize);

		dist = FractionalAverage(neighbors, neighborSize, i - start, &avg);
		free(neighbors);

		if(dist < threshold){
			(*melodyMidi)[i] = round(roundf(fractNotes[i] - avg) + avg);
		}else{
			(*melodyMidi)[i] = round(fractNotes[i]);
		}
	}

	return num_notes;
}		

int NumSTFTBlocks(audioInfo info, int unpaddedSize, int interval)
{
	int numBlocks = (int)ceil((info.frames - unpaddedSize)
				  / (float)interval) + 1;
	if (numBlocks < 1){
		numBlocks = 1;
	}
	return numBlocks;
}

//reads in .wav, returns FFT by reference through fft_data, returns size of fft_data
int STFT_r2c(float** input, audioInfo info, int unpaddedSize, int winSize, int interval, fftwf_complex** fft_data)
{
	//printf("\n\ninput size %ld\nsamplerate %d\nunpadded %d\npadded %d\ninterval %d\n", info.frames, info.samplerate, unpaddedSize, winSize, interval);
	//fflush(NULL);
    int i;
    int j;

    //malloc space for result fft_data
	int numBlocks = NumSTFTBlocks(info, unpaddedSize, interval);
	//printf("numblocks %d\n", numBlocks);
	//fflush(NULL);

	//allocate winSize/2 for each block, taking the real component 
	//and dropping the symetrical component and nyquist frequency.
	int realWinSize = winSize/2;

    (*fft_data) = malloc( sizeof(fftwf_complex) * numBlocks * realWinSize );
    if((*fft_data) == NULL){
    	printf("malloc failed\n");
		return -1;
    }

    //set up fftw plan
    float* fftw_in = fftwf_malloc( sizeof( float ) * winSize);
    fftwf_complex* fftw_out = fftwf_malloc( sizeof( fftwf_complex ) * winSize );
    fftwf_plan plan  = fftwf_plan_dft_r2c_1d( winSize, fftw_in, fftw_out, FFTW_MEASURE );

    float* window = WindowFunction(winSize);
    if(window == NULL){
    	printf("windowFunc error\n");
    	fflush(NULL);
    	free((*fft_data));
    	fftwf_destroy_plan( plan );
		fftwf_free( fftw_in );
		fftwf_free( fftw_out );
		return -1;
    }

 	//printf("resultsize %ld\nresultWinSize %d\n", sizeof(fftwf_complex) * numBlocks * realWinSize, realWinSize);
	//fflush(NULL);
	int blockoffset;
    //run fft on each block
    for(i = 0; i < numBlocks; i++){
		// Copy the chunk into our buffer
		blockoffset = i*interval;
		for(j = 0; j < winSize; j++) {

			if(j < unpaddedSize && blockoffset + j < info.frames) {
				fftw_in[j] = (*input)[blockoffset + j] * window[j]; 
			} else {
				//reached end of non-padded input
				//Pad the rest with 0
				fftw_in[j] = 0.0; 
			}
		}

		fftwf_execute( plan );

		for (j = 0; j < realWinSize; j++) {
			(*fft_data)[i*realWinSize + j][0] = fftw_out[j][0];
			(*fft_data)[i*realWinSize + j][1] = fftw_out[j][1];
		}	
	}


	free(window);
	fftwf_destroy_plan( plan );
	fftwf_free( fftw_in );
	fftwf_free( fftw_out );

	return numBlocks * realWinSize;
}

int STFTinverse_c2r(fftwf_complex** input, audioInfo info, int winSize, int interval, float** output)
{
	//length of input is numBlocks * (winSize/2 + 1)
    int i;
    int j;
	int inputoffset;
	int outputoffset;

    fftwf_complex* fftw_in = fftwf_malloc( sizeof( fftwf_complex ) * winSize );
	float* fftw_out = fftwf_malloc( sizeof( float ) * winSize );

    fftwf_plan plan  = fftwf_plan_dft_c2r_1d( winSize, fftw_in, fftw_out, FFTW_MEASURE );

    //float* window = WindowFunction(winSize+1);

 	//malloc space for output
    (*output) = calloc( info.frames, sizeof(float));
        if((*output) == NULL){
    	printf("malloc failed\n");
    	//free(window);
		fftwf_destroy_plan( plan );
		fftwf_free( fftw_in );
		fftwf_free( fftw_out );
		return -1;
    }

	int numBlocks = (int)ceil((info.frames - winSize) / (float)interval) + 1;
	if(numBlocks < 1){
		numBlocks = 1;
	}

    //run fft on each block
    for(i = 0; i < numBlocks; i++){

		// Copy the chunk into our buffer
		inputoffset = i*(winSize/2 + 1);
		for(j = 0; j < winSize/2 + 1; j++) {
			fftw_in[j][0] = (*input)[inputoffset + j][0]; 
			fftw_in[j][1] = (*input)[inputoffset + j][1]; 
		}

		fftwf_execute( plan );

		outputoffset = i*interval;

		for (j = 0; j < winSize; j++) {
			if(outputoffset + j < info.frames) {
				(*output)[outputoffset + j] += fftw_out[j] / (float)((winSize*winSize)/interval);
			}
		}	
	}

	fftwf_destroy_plan( plan );
	fftwf_free( fftw_in );
	fftwf_free( fftw_out );

	return info.frames;
}

float* Magnitude(fftwf_complex* arr, int size)
{
	int i;
	float* magArr = malloc( sizeof(float) * size);
	if (magArr != NULL){
		for(i = 0; i < size; i++){
			magArr[i] = hypot(arr[i][0], arr[i][1]);
		}
	}
	return magArr;
}

void SaveWeightsTxt(char* fileName, float** AudioData, int size, int dftBlocksize, int samplerate, int unpaddedSize, int winSize){
	// Saves the weights of each frequency bin to a text file
	FILE *fp;
	int blockstart, i;
	fp = fopen(fileName,"w");

	fprintf(fp, "#Window Size:\t%d\n", winSize);
	fprintf(fp, "#Window Size Before Zero Padding:\t%d\n", unpaddedSize);
	fprintf(fp, "#Sample Rate:\t%d\n", samplerate);
	// outer loop iterates over blocks
	for(blockstart = 0; blockstart < size; blockstart += dftBlocksize){
		//iterate over elements of a block
		for(i = 0; i < dftBlocksize; ++i){
			fprintf(fp, "%e\t", (*AudioData)[blockstart + i]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}


void SaveNotesTxt(char* fileName, int* noteRanges, int* notePitches,
		  int nP_size, int samplerate){
	// Saves the notes and note pitches. This is for use while debugging
	
	FILE *fp;
	int i;
	fp = fopen(fileName,"w");
	
	// write out important comments
	fprintf(fp, "#Sample Rate:\t%d\n", samplerate);
	fprintf(fp, "#note_start and note stop are in units of num of samples \n");
	fprintf(fp, "#note_pitch is the midi num associated with the pitch\n");
	
	// write out the header:
	fprintf(fp, "note_start\tnote_stop\tnote_pitch");

	// write out the notes:
	for(i =0; i<nP_size; i++){
		fprintf(fp, "\n%d\t%d\t%d",noteRanges[2*i], noteRanges[2*i+1],
			notePitches[i]);
	}

	fclose(fp);
}
