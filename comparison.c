#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
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
	//size = blocksize+1
	float* buffer = malloc(sizeof(float) * size);
	for(int i = 0; i < size; ++i) {
		buffer[i] =(float) (0.54-(0.46*cos(2*M_PI*i/(size-1.0))));
	}
	return buffer;
}

int ExtractMelody(char* filename)
{ 
	int sampleSize = 2048;
	int interval = 50;

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
	fftw_free( input );

	printf("numblcks: %d\nlast val: %f %f\n", size/(sampleSize/2 + 1), AudioData[size-1][0], AudioData[size-1][1]);


	//free data
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