#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "extractMelodyProcedure.h"
#include "pitch/pitchStrat.h"
#include "transient/transient.h"
#include "stft.h"
#include "midi.h"
#include "silenceStrat.h"
#include "winSampleConv.h"
#include "noteCompilation.h"
#include "tuningAdjustment.h"
#include "errors.h"


int  ExtractMelody(float** input, audioInfo info,
		   int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		   int s_winSize, int s_winInt, int s_mode, SilenceStrategyFunc silenceStrategy,
		   int hpsOvr, int tuning, int verbose, char* prefix,
		   FILE* f)
{

	if(verbose){
		printf("ARGS:\n");
		printf("p_unpad %d,  p_win %d,  p_int %d\n", p_unpaddedSize, p_winSize, p_winInt);
		printf("s_win %d,  s_int %d,  s_mode %d\n", s_winSize, s_winInt, s_mode);
		printf("hps %d,  tuning %d,  verbose %d,  prefix %s\n", hpsOvr, tuning, verbose, prefix);
	}

	int *activityRanges = NULL;
	int a_size = ExtractSilence(input, &activityRanges, info, s_winSize,
				    s_winInt, s_mode, silenceStrategy);
	if(a_size < 0){
		printf("Silence detection failed\n");
		fflush(NULL);
		return a_size;
	} else if(verbose){
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
		return (freqSize < 0) ? freqSize : ME_ERROR;
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
	intList* transients = intListCreate(20);

	int t_size = DetectTransientsFromResampled(*input, info.frames,
						   info.samplerate, transients);
	if(t_size <= 0){
		printf("Onset detection failed\n");
		fflush(NULL);
		free(activityRanges);
		free(freq);
		intListDestroy(transients);
		return (t_size < 0) ? t_size : ME_ERROR;
	} else if(verbose){
		printf("Onset detection complete\n");
		fflush(NULL);
	}


	int *noteRanges = NULL;
	float *noteFreq = NULL;
	int num_notes = ConstructNotes(&noteRanges, &noteFreq, freq,
				     freqSize, transients, t_size, activityRanges,
				     a_size, info, p_unpaddedSize, p_winInt);

	free(activityRanges);
	free(freq);
	intListDestroy(transients);

	if(num_notes <= 0){
		int exit_code = ME_ERROR;
		if (num_notes == 0){
			printf("No notes detected.\n");
		} else {
			printf("Construct notes failed!\n");
			exit_code = num_notes;
		}
		fflush(NULL);
		return exit_code;
	}
	printf("construct notes\n");

	int* melodyMidi = malloc(sizeof(int) * num_notes);
	if(melodyMidi == NULL){
		printf("malloc failed\n");
		fflush(NULL);
		free(noteRanges);
		free(noteFreq);
		return ME_MALLOC_FAILURE;
	}
	int tmp = FrequenciesToNotes(noteFreq, num_notes, &melodyMidi, tuning);
	if(tmp <= 0){
		printf("freqToNote failed\n");
		fflush(NULL);
		free(noteRanges);
		free(noteFreq);
		free(melodyMidi);
		return (tmp < 0) ? tmp : ME_ERROR;
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

	PrintDetectionSummary(info, noteRanges, noteFreq, melodyMidi,
			      num_notes);

	free(noteFreq);

	//get midi note values of pitch in each bin

	// TODO: consolidate all of the following into 1 function!
	int midi_exit_code;
	midi_exit_code = WriteNotesAsMIDI(melodyMidi, noteRanges,
					  num_notes, info.samplerate, f,
					  verbose);

	free(noteRanges);
	free(melodyMidi);

	if (midi_exit_code != ME_SUCCESS) {
		printf("Midi generation failed\n");
		fflush(NULL);
		return midi_exit_code;
	}

	return ME_SUCCESS;
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

void PrintDetectionSummary(audioInfo info, const int * noteRanges,
			   const float * noteFreq, const int * melodyMidi,
			   int num_notes)
{
	printf("Detected %d Notes. Printing Summary:\n", num_notes);
	// It would be better if this were somewhat adaptive

	const char * hdr[6][2] = { {"Start - Stop ", "(samples) "},
				   {"Start - Stop ", "(nearest ms) "},
				   {"Frequency", "(Hz)"},
				   {"Raw MIDI", "Pitch"},
				   {"Final", "Pitch"},
				   {"Pitch", "Name"} };
	const char* hdr_template = "%18s|%17s|%9s|%8s|%5s|%5s\n";
	printf(hdr_template, hdr[0][0], hdr[1][0], hdr[2][0], hdr[3][0],
	       hdr[4][0], hdr[5][0]);
	printf(hdr_template, hdr[0][1], hdr[1][1], hdr[2][1], hdr[3][1],
	       hdr[4][1], hdr[5][1]);
	printf("==================+=================+=========+========+=====+=====\n");

	for(int i =0; i<num_notes; i++){
		printf("%7d - %7d | ", noteRanges[2*i], noteRanges[2*i+1]);
		int start_ms = (int)(noteRanges[2*i] *
				     (1000.0/info.samplerate) + 0.5);
		int stop_ms = (int)(noteRanges[2*i+1] *
				    (1000.0/info.samplerate) + 0.5);
		printf("%6d - %6d | ", start_ms, stop_ms);
		printf("%7.2f | ", noteFreq[i]);
		printf("%6.2f | ", FrequencyToFractionalNote(noteFreq[i]));
		printf("%3d | ", melodyMidi[i]);
		char noteName[5];
		NoteToName(melodyMidi[i], noteName);
		printf("%s\n", noteName);
	}

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
