#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include "sndfile.h"
#include "fftw3.h"
#include "comparison.h"
#include "midi.h"
#include "pitchStrat.h"
#include "onsetStrat.h"
#include "silenceStrat.h"
#include "winSampleConv.h"
#include "noteCompilation.h"

void PrintAudioMetadata(SF_INFO * file)
{
	//this is only for printing information for debugging purposes
	printf("Frames:\t%ld\n", file->frames); 
	printf("Sample rate:\t%d\n", file->samplerate);
	printf("Channels: \t%d\n", file->channels);
	printf("Format: \t%d\n", file->format);
	printf("Sections: \t%d\n", file->sections);
	printf("Seekable: \t%d\n", file->seekable);
}

float* WindowFunction(int size)
{
	//uses Hamming Window
	float* buffer = malloc(sizeof(float) * size);
	for(int i = 0; i < size; ++i) {
		buffer[i] =(float) (0.54-(0.46*cos(2*M_PI*i/(size-1.0))));
	}
	return buffer;
}

struct Midi* ExtractMelody(float** input, SF_INFO info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int o_unpaddedSize, int o_winSize, int o_winInt, OnsetStrategyFunc onsetStrategy,
		int s_winSize, int s_winInt, int s_mode, SilenceStrategyFunc silenceStrategy,
		int hpsOvr, int verbose, char* prefix)
{ 
	if(verbose){
		PrintAudioMetadata(&info);
	}

	int *activityRanges = NULL;
	int a_size = ExtractSilence(input, &activityRanges, info, s_winSize,
				    s_winInt, s_mode, silenceStrategy);
	
	if(verbose){
		printf("Silence detection complete\n");
		fflush(NULL);
	}
	float* freq;
	int freqSize = ExtractPitch(input, &freq, info, p_unpaddedSize, p_winSize,
	                          p_winInt, pitchStrategy, hpsOvr, verbose, prefix);
	if(verbose){
		printf("Pitch detection complete\n");
		fflush(NULL);
	}

	int *onsets = NULL;
	int o_size = ExtractOnset(input, &onsets, info, o_unpaddedSize, o_winSize, o_winInt, 
	             onsetStrategy, verbose);
	
	if(verbose){
		printf("Onset detection complete\n");
		fflush(NULL);
	}


	int *noteRanges = NULL;
	float *noteFreq = NULL;
	int num_notes = ConstructNotes(&noteRanges, &noteFreq, freq,
				     freqSize, onsets, o_size, activityRanges,
				     a_size, info, p_unpaddedSize, p_winInt);

	int* melodyMidi = malloc(sizeof(float) * num_notes);

	for(int i = 0; i < num_notes; ++i){
		melodyMidi[i] = FrequencyToNote(noteFreq[i]);
	}

	char* noteName = calloc(5, sizeof(char));
	printf("Detected %d Notes:\n", num_notes);
	for(int i =0; i<num_notes; i++){
		NoteToName(melodyMidi[i], &noteName);
		printf("%d - %d,   %d ms - %d ms,   %.2f hz,   %s\n",
		       noteRanges[2*i], noteRanges[2*i+1],
		       (int)(noteRanges[2*i] * (1000.0/info.samplerate)),
		       (int)(noteRanges[2*i+1] * (1000.0/info.samplerate)),
		       noteFreq[i], noteName);
	}

	free(activityRanges);
	free(onsets);
	free(noteFreq);
	
	//get midi note values of pitch in each bin

	printf("printout complete\n");
	fflush(NULL);

	//todo: restructure generateMidi function to also take noteRanges
	struct Midi* midi = GenerateMIDI(melodyMidi, num_notes, verbose);

	free(noteRanges);

	return midi;
}

int ExtractPitch(float** input, float** pitches, SF_INFO info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int hpsOvr, int verbose, char* prefix)
{
	fftwf_complex* p_fftData = NULL;
	int p_size = STFT_r2c(input, info, p_unpaddedSize, p_winSize, p_winInt, &p_fftData);
	int p_numBlocks = p_size/(p_winSize/2);
	if(verbose){
		printf("numblcks of pitch FFT: %d\n", p_numBlocks);
		fflush(NULL);
	}

	float* spectrum = Magnitude(p_fftData, p_size);
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
	
	(*pitches) = pitchStrategy(&spectrum, p_size, p_winSize/2, hpsOvr,
					p_winSize, info.samplerate);

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

int ExtractSilence(float** input, int** activityRanges, SF_INFO info,
		   int s_winSize, int s_winInt, int s_mode,
		   SilenceStrategyFunc silenceStrategy){
	int a_size = silenceStrategy(input, info.frames, s_winSize, s_winInt,
				     info.samplerate, s_mode,
				     activityRanges);
	for (int i = 0; i < a_size; i++){
		printf("Activity Range: %d up to %d\n", (*activityRanges)[i],
		       (*activityRanges)[i+1]);
		i++;
	}
	return a_size;
}

int ExtractOnset(float** input, int** onsets, SF_INFO info, int o_unpaddedSize, int o_winSize, 
                  int o_winInt, OnsetStrategyFunc onsetStrategy, int verbose)
{
	//todo: add 'onset threshold' parameter.
	//if two onsets are found with less than 'threshold' time between them, ignore the second one as a false positive.
	//i think a good default would be 40ms.
	fftwf_complex* o_fftData = NULL;
	int o_fftData_size = STFT_r2c(input, info, o_unpaddedSize, o_winSize, o_winInt, &o_fftData);
	int o_numBlocks = o_fftData_size/(o_winSize/2);
	if(verbose){
		printf("numblcks of onset FFT: %d\n", o_numBlocks);
		fflush(NULL);
	}
	float* o_fftData_float = (float*)o_fftData;
	o_fftData_size *= 2; //because we convert from complex* to float*, o_fftData has twice as many elements

	int o_size = onsetStrategy(&o_fftData_float, o_fftData_size, o_winSize, info.samplerate, onsets);
	//printf("o_strat return size: %d\n", o_size);
	free(o_fftData);


	//convert onsets so that the value is not which block of o_fftData it occurred, but which sample of the original audio it occurred
	for (int i = 0; i < o_size; i++){
		(*onsets)[i] = winStartRepSampleIndex(o_winInt, o_unpaddedSize,
						      info.frames,
						      (*onsets)[i]);
		printf("onset at sample: %d, time: %d, and block: %d\n",
		       (*onsets)[i],
		       (int)((*onsets)[i] * (1000.0/info.samplerate)),
		       repWinIndex(o_winInt, o_unpaddedSize, info.frames,
				   (*onsets)[i]));
	}

	return o_size;
}

int ConstructNotes(int** noteRanges, float** noteFreq, float* pitches,
		   int p_size, int* onsets, int onset_size, int* activityRanges,
		   int aR_size, SF_INFO info, int p_unpaddedSize, int p_winInt)
{
	int nR_size = calcNoteRanges(onsets, onset_size, activityRanges,
				     aR_size, noteRanges, info.samplerate);

	int nF_size = assignNotePitches(pitches, p_size, *noteRanges, nR_size,
					p_winInt, p_unpaddedSize, info.frames,
					noteFreq);

	return nF_size;
}

		

//reads in .wav, returns FFT by reference through fft_data, returns size of fft_data
int STFT_r2c(float** input, SF_INFO info, int unpaddedSize, int winSize, int interval, fftwf_complex** fft_data)
{
    int i;
    int j;

    float* fftw_in = fftwf_malloc( sizeof( float ) * winSize);
    fftwf_complex* fftw_out = fftwf_malloc( sizeof( fftwf_complex ) * winSize );

    fftwf_plan plan  = fftwf_plan_dft_r2c_1d( winSize, fftw_in, fftw_out, FFTW_MEASURE );

    float* window = WindowFunction(winSize);
 
    //malloc space for fft_data
	int numBlocks = (int)ceil((info.frames - unpaddedSize) / (float)interval) + 1;
	if(numBlocks < 1){
		numBlocks = 1;
	}
	//allocate winSize/2 for each block, taking the real component 
	//and dropping the symetrical component and nyquist frequency.
	int realWinSize = winSize/2;
    (*fft_data) = malloc( sizeof(fftwf_complex) * numBlocks * realWinSize );
 
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

int STFTinverse_c2r(fftwf_complex** input, SF_INFO info, int winSize, int interval, float** output)
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
	for(i = 0; i < size; i++){
		magArr[i] = hypot(arr[i][0], arr[i][1]);
	}
	return magArr;
}



void SaveAsWav(const double* audio, SF_INFO info, const char* path) {
	FILE* file = fopen(path, "wb");
	
	//Encode samples to 16 bit
	short* samples = (short*)malloc(info.frames* sizeof(short));
	unsigned int i;
	short max = -10000;
	int maxloc = 0;
	short min = 10000;
	int minloc = 0;
	for (i = 0; i < info.frames; ++i) {
		samples[i] = htole16(fmin(fmax(audio[i], -1), 1) * SHRT_MAX);
		if(samples[i] > max){
			max = samples[i];
			maxloc = i;
		}
		if(samples[i] < min){
			min = samples[i];
			minloc = i;
		}
	}

	printf("\tmax: %d at %d\tmin: %d at %d\n", max, maxloc, min, minloc);
	
	//Heaader chunk
	fprintf(file, "RIFF");
	unsigned int chunksize = htole32((info.frames * sizeof(short)) + 36);
	fwrite(&chunksize, sizeof(chunksize), 1, file);
	fprintf(file, "WAVE");
	
	//Format chunk
	fprintf(file, "fmt ");
	unsigned int fmtchunksize = htole32(16);
	fwrite(&fmtchunksize, sizeof(fmtchunksize), 1, file);
	unsigned short audioformat = htole16(1);
	fwrite(&audioformat, sizeof(audioformat), 1, file);
	unsigned short numchannels = htole16(1);
	fwrite(&numchannels, sizeof(numchannels), 1, file);
	unsigned int samplerate = htole32(info.samplerate);
	fwrite(&samplerate, sizeof(samplerate), 1, file);
	unsigned int byterate = htole32(samplerate * numchannels * sizeof(short));
	fwrite(&byterate, sizeof(byterate), 1, file);
	unsigned short blockalign = htole16(numchannels * sizeof(short));
	fwrite(&blockalign, sizeof(blockalign), 1, file);
	unsigned short bitspersample = htole16(sizeof(short) * CHAR_BIT);
	fwrite(&bitspersample, sizeof(bitspersample), 1, file);
	
	//Data chunk
	fprintf(file, "data");
	unsigned int datachunksize = htole32(info.frames * sizeof(short));
	fwrite(&datachunksize, sizeof(datachunksize), 1, file);
	fwrite(samples, sizeof(short), info.frames, file);
	
	//Free encoded samples
	free(samples);
	
	//Close the file
	fclose(file);
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
