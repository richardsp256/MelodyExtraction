#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include "sndfile.h"
#include "fftw3.h"
#include "comparison.h"

void PrintAudioMetadata(SF_INFO * file)
{
	//this is only for printing information for debugging purposes
    printf("Frames:\t%ld\n", file->frames); 
    printf("Sample rate:\t%d\n", file->samplerate);
    printf("Channels: \t%d\n", file->channels); //comparison wont be accurate if channels is not 0! 
    printf("Format: \t%d\n", file->format);
    printf("Sections: \t%d\n", file->sections);
    printf("Seekable: \t%d\n", file->seekable);
}

float* WindowFunction(int size)
{
	//uses Hamming Window
	//note: size = blocksize+1
	float* buffer = malloc(sizeof(float) * size);
	for(int i = 0; i < size; ++i) {
		buffer[i] =(float) (0.54-(0.46*cos(2*M_PI*i/(size-1.0))));
	}
	return buffer;
}

int ExtractMelody(char* filename)
{ 
	int sampleSize = 2048;
	int interval = 1;

	//reads in .wav
	SF_INFO info;
	SNDFILE * f = sf_open(filename, SFM_READ, &info);
	if( !f ){
		printf("ERROR: could not open %s for processing\n", filename );
		return -1;
	}
	if(info.channels != 1){
		printf("ERROR: .wav file must be Mono. Code can't currently handle multi-channel audio.\n");
		sf_close( f );
		return -1;
	}
	
	PrintAudioMetadata(&info);

	double* input = fftw_malloc( sizeof(double) * info.frames);
	sf_count_t numRead = sf_readf_double( f, input, info.frames );
	if ( numRead != info.frames ) {
		printf("ERROR: input received incorrect amount of data\n");
		return -1;
	}
	sf_close( f );

	double** AudioData = NULL;
	int size = STFT(&input, info, sampleSize, interval, &AudioData);
	printf("numblcks: %d\n", size/(sampleSize/2 + 1));

	double* output = NULL;
	size = STFTinverse(&AudioData, info, sampleSize, interval, &output);

	char* fname3 = "Undertale_2.wav";
	SaveAsWav(output, info, fname3);

	//free data
	fftw_free( input );
	fftw_free( output );
	
	int i;
	for(i = 0; i < size; i++){
		free(AudioData[i]);
	}
	free(AudioData);

	return size;
}

//reads in .wav, returns FFT by reference through dft_data, returns size of dft_data
int STFT(double** input, SF_INFO info, int blocksize, int interval, double*** dft_data) {
    int i;
    int j;

    double* fftw_in = fftw_malloc( sizeof( double ) * blocksize * info.channels );
    fftw_complex* fftw_out = fftw_malloc( sizeof( fftw_complex ) * blocksize );

    fftw_plan plan  = fftw_plan_dft_r2c_1d( blocksize, fftw_in, fftw_out, FFTW_MEASURE );

    float* window = WindowFunction(blocksize+1);
 
    //malloc space for dft_data
	int numBlocks = (int)(ceil(((info.frames - (double)blocksize) / (double)interval) + 1));
	if(numBlocks < 1){
		numBlocks = 1;
	}
    (*dft_data) = malloc( sizeof(double*) * numBlocks * (blocksize/2 + 1) );
	for(i = 0; i < (numBlocks * (blocksize/2 + 1)); i++){
		(*dft_data)[i] = malloc(sizeof(double) * 2);
	}
 
	int blockoffset;
    //run fft on each block
    for(i = 0; i < numBlocks; i++){

		// Copy the chunk into our buffer
		blockoffset = i*interval;
		for(j = 0; j < blocksize; j++) {

			if(blockoffset + j < info.frames) {
				fftw_in[j] = (*input)[blockoffset + j] * window[j]; 
			} else {
				//reached end of input
				//Pad with 0 so fft is same size as other blocks
				fftw_in[j] = 0.0; 
			}
		}

		fftw_execute( plan );

		for (j = 0; j < blocksize/2 + 1; j++) {
			(*dft_data)[i*(blocksize/2 + 1) + j][0] = fftw_out[j][0];
			(*dft_data)[i*(blocksize/2 + 1) + j][1] = fftw_out[j][1];
		}	
	}

	fftw_destroy_plan( plan );
	fftw_free( fftw_in );
	fftw_free( fftw_out );

	return numBlocks * (blocksize/2 + 1);
}

int STFTinverse(double*** input, SF_INFO info, int blocksize, int interval, double** output) {
	//length of input is numBlocks * (blocksize/2 + 1)
    int i;
    int j;
	int inputoffset;
	int outputoffset;

    fftw_complex* fftw_in = fftw_malloc( sizeof( fftw_complex ) * blocksize );
	double* fftw_out = fftw_malloc( sizeof( double ) * blocksize * info.channels );

    fftw_plan plan  = fftw_plan_dft_c2r_1d( blocksize, fftw_in, fftw_out, FFTW_MEASURE );

    //float* window = WindowFunction(blocksize+1);
 
    //malloc space for output
    (*output) = fftw_malloc( sizeof(double) * info.frames );

	int numBlocks = (int)(ceil(((info.frames - (double)blocksize) / (double)interval) + 1));
	if(numBlocks < 1){
		numBlocks = 1;
	}

    //run fft on each block
    for(i = 0; i < numBlocks; i++){

		// Copy the chunk into our buffer
		inputoffset = i*(blocksize/2 + 1);
		for(j = 0; j < blocksize/2 + 1; j++) {
			fftw_in[j][0] = (*input)[inputoffset + j][0]; 
			fftw_in[j][1] = (*input)[inputoffset + j][1]; 
		}

		fftw_execute( plan );

		outputoffset = i*interval;

		for (j = 0; j < blocksize; j++) {
			if(outputoffset + j < info.frames) {
				(*output)[outputoffset + j] += (fftw_out[j] / (double)((blocksize*blocksize)/interval));
			}
		}	
	}

	fftw_destroy_plan( plan );
	fftw_free( fftw_in );
	fftw_free( fftw_out );

	return info.frames;
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