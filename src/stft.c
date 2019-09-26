#include <stdlib.h>
#include <math.h>
#include "fftw3.h"
#include "melodyextraction.h"

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

int NumSTFTBlocks(audioInfo info, int unpaddedSize, int interval)
{
	int numBlocks = (int)ceil((info.frames - unpaddedSize)
				  / (float)interval) + 1;
	if (numBlocks < 1){
		numBlocks = 1;
	}
	return numBlocks;
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