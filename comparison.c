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

const char* ERR_INVALID_FILE = "Audio file could not be opened for processing\n";
const char* ERR_FILE_NOT_MONO = "Input file must be Mono."
                          " Multi-channel audio currently not supported.\n";

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

int ExtractMelody(char* inFile, char* outFile, 
		int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int o_winSize, int o_winInt, OnsetStrategyFunc onsetStrategy,
		int hpsOvr, int verbose, char* prefix)
{ 
	//reads in .wav
	SF_INFO info;
	SNDFILE * f = sf_open(inFile, SFM_READ, &info);
	if( !f ){
		printf("%s", ERR_INVALID_FILE);
		return -1;
	}
	if(info.channels != 1){
		printf("%s", ERR_FILE_NOT_MONO);
		sf_close( f );
		return -1;
	}
	
	if(verbose){
		PrintAudioMetadata(&info);
	}

	float* input = malloc( sizeof(float) * info.frames);
	sf_readf_float( f, input, info.frames );
	sf_close( f );

	fftwf_complex* fftData = NULL;
	int size = STFT_r2c(&input, info, p_winSize, p_winInt, &fftData);
	int numBlocks = size/(p_winSize/2);
	if(verbose){
		printf("numblcks of FFT: %d\n", numBlocks);
		fflush(NULL);
	}

	double* spectrum = Magnitude(fftData, size);
	if(verbose){
		printf("Magnitude complete\n");
		fflush(NULL);
	}

	if (prefix !=NULL){
		// Here we save the original spectra
		char *spectraFile = malloc(sizeof(char) * (strlen(prefix)+14));
		strcpy(spectraFile,prefix);
		strcat(spectraFile,"_original.txt");
		SaveWeightsTxt(spectraFile, &spectrum, size, p_winSize/2, info.samplerate, p_winSize);
		free(spectraFile);
	}
	
	float* freq = pitchStrategy(&spectrum, size, p_winSize/2, hpsOvr,
					p_winSize, info.samplerate);

	if(verbose){
		printf("HPS complete\n");
		fflush(NULL);
	}

	int* melodyMidi = malloc(sizeof(float) * numBlocks);

	if (prefix !=NULL){
		// Here we save the processed spectra
		char *spectraFile = malloc(sizeof(char) * (strlen(prefix)+14));
		strcpy(spectraFile,prefix);
		strcat(spectraFile,"_weighted.txt");
		SaveWeightsTxt(spectraFile, &spectrum, size, p_winSize/2, info.samplerate, p_winSize);
		free(spectraFile);
	}
	
	//get midi note values of pitch in each bin
	for(int i = 0; i < numBlocks; ++i){
		if(verbose){
			printf("block%d:", i);
			fflush(NULL);
			printf("   freq: %.2f", freq[i]);
			fflush(NULL);
		}
		
		melodyMidi[i] = FrequencyToNote(freq[i]);
		if(verbose){
			printf("   midi: %d", melodyMidi[i]);
			fflush(NULL);
		}
		char* noteName = calloc(5, sizeof(char));
		NoteToName(melodyMidi[i], &noteName);
		if(verbose){
			printf("   name: %s \n", noteName);
			fflush(NULL);
		}
		free(noteName);
	}

	printf("printout complete\n");
	fflush(NULL);

	SaveMIDI(melodyMidi, numBlocks, outFile, verbose);

	free(melodyMidi);


	//double* output = NULL;
	//STFTinverse(&fftData, info, winSize, p_winInt, &output);
	//if(verbose){
	//	printf("STFTInverse complete\n");
	//}
	free(fftData);

	//SaveAsWav(output, info, outFile);

	/*
	 * if input is free before STFTinverse is called, result changes...
	 */
	free( input );
	//free( output );

	return info.frames;
}

//reads in .wav, returns FFT by reference through fft_data, returns size of fft_data
int STFT_r2c(float** input, SF_INFO info, int winSize, int interval, fftwf_complex** fft_data)
{
    int i;
    int j;

    float* fftw_in = fftwf_malloc( sizeof( float ) * winSize);
    fftwf_complex* fftw_out = fftwf_malloc( sizeof( fftwf_complex ) * winSize );

    fftwf_plan plan  = fftwf_plan_dft_r2c_1d( winSize, fftw_in, fftw_out, FFTW_MEASURE );

    float* window = WindowFunction(winSize);
 
    //malloc space for fft_data
	int numBlocks = (int)ceil((info.frames - winSize) / (float)interval) + 1;
	if(numBlocks < 1){
		numBlocks = 1;
	}
	//allocate winsize/2 for each block, taking the real component 
	//and dropping the complex component and nyquist frequency.
	int realWinSize = winSize/2;
    (*fft_data) = malloc( sizeof(fftwf_complex) * numBlocks * realWinSize );
 
	int blockoffset;
    //run fft on each block
    for(i = 0; i < numBlocks; i++){

		// Copy the chunk into our buffer
		blockoffset = i*interval;
		for(j = 0; j < winSize; j++) {

			if(blockoffset + j < info.frames) {
				fftw_in[j] = (*input)[blockoffset + j] * window[j]; 
			} else {
				//reached end of input
				//Pad with 0 so fft is same size as other blocks
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

//reads in .wav, returns FFT by reference through fft_data, returns size of fft_data
int STFT_r2r(float** input, SF_INFO info, int winSize, int interval, float** fft_data)
{
    int i;
    int j;

    float* fftw_in = fftwf_malloc( sizeof( float ) * winSize);
    float* fftw_out = fftwf_malloc( sizeof( fftwf_complex ) * winSize );

    fftwf_plan plan  = fftwf_plan_r2r_1d( winSize, fftw_in, fftw_out, FFTW_R2HC, FFTW_MEASURE );

    float* window = WindowFunction(winSize);
 
    //malloc space for fft_data
	int numBlocks = (int)ceil((info.frames - winSize) / (float)interval) + 1;
	if(numBlocks < 1){
		numBlocks = 1;
	}

	//allocate winsize for each block, storing the real and imaginary components up to the nyquist
    (*fft_data) = malloc( sizeof(float) * numBlocks * winSize );
 
	int blockoffset;
    //run fft on each block
    for(i = 0; i < numBlocks; i++){

		// Copy the chunk into our buffer
		blockoffset = i*interval;
		for(j = 0; j < winSize; j++) {

			if(blockoffset + j < info.frames) {
				fftw_in[j] = (*input)[blockoffset + j] * window[j]; 
			} else {
				//reached end of input
				//Pad with 0 so fft is same size as other blocks
				fftw_in[j] = 0.0; 
			}
		}

		fftwf_execute( plan );

		for (j = 0; j < winSize; j++) {
			(*fft_data)[i*winSize + j] = fftw_out[j];
		}	
	}

	free(window);
	fftwf_destroy_plan( plan );
	fftwf_free( fftw_in );
	fftwf_free( fftw_out );

	return numBlocks * winSize;
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

double* Magnitude(fftwf_complex* arr, int size)
{
	int i;
	double* magArr = malloc( sizeof(double) * size);
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

void SaveWeightsTxt(char* fileName, double** AudioData, int size, int dftBlocksize, int samplerate, int winSize){
        // Saves the weights of each frequency bin to a text file
        FILE *fp;
	int blockstart, i;
	fp = fopen(fileName,"w");

	fprintf(fp, "#Window Size:\t%d\n", winSize);
	fprintf(fp, "#Sample Rate:\t%d\n", samplerate);
	// outer loop iterates over blocks
	for(blockstart = 0; blockstart < (size - dftBlocksize); blockstart += dftBlocksize){
		//iterate over elements of a block
		for(i = 0; i < dftBlocksize; ++i){
			fprintf(fp, "%e\t", (*AudioData)[blockstart + i]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}
