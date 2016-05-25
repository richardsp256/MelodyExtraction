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

int ExtractMelody(char* filename)
{ 
	int sampleSize = 2048;

	//reads in .wav
	SF_INFO info;
	SNDFILE * f = sf_open(filename, SFM_READ, &info);
	if( !f ){
		printf("ERROR: could not open %s for processing\n", filename );
		return 0;
	}
	if(info.channels != 1){
		printf("ERROR: .wav file must be Mono. Code can't currently handle multi-channel audio.\n");
		sf_close( f );
		return 0;
	}
	PrintAudioMetadata(&info);

	//run STFT on input
	double** AudioData = NULL;
	int size = STFT(f, info, sampleSize, &AudioData);
	if( !size ) {
		printf("ERROR: ReadAudioFile failed for %s\n", filename);
	}
	
	printf("size is %d\n", size);

	//free data
	int i;
	for(i = 0; i < size; i++){
		free(AudioData[i]);
	}
	free(AudioData);

	return size;
}

int STFT(SNDFILE * f, SF_INFO info, int blocksize, double*** dft_data)
{
	//reads in .wav, returns FFT by reference through dft_data, returns size of dft_data
	sf_count_t i;
	sf_count_t j;

	double* fftw_in = fftw_malloc( sizeof(double) * blocksize * info.channels );
	if ( !fftw_in ) {
		printf("ERROR: fftw_malloc 1 failed\n");
		sf_close( f );
		return 0;
	}

	fftw_complex* fftw_out = fftw_malloc( sizeof(fftw_complex) * blocksize );
	if ( !fftw_out ) {
		printf("ERROR: fftw_malloc 2 failed\n");
		fftw_free( fftw_in );
		sf_close( f );
		return 0;
	}

	fftw_plan plan = fftw_plan_dft_r2c_1d( blocksize, fftw_in, fftw_out, FFTW_MEASURE );
	if ( !plan ) {
		printf("ERROR: Could not create plan\n");
		fftw_free( fftw_in );
		fftw_free( fftw_out );
		sf_close( f );
		return 0;
	}

	//allocate space for array to return
	int numBlocks = (int)(ceil(info.frames / (double)blocksize));
    (*dft_data) = malloc( sizeof(double*) * numBlocks * blocksize/2 );
	for(i = 0; i < (numBlocks * blocksize / 2.0); i++){
		(*dft_data)[i] = malloc(sizeof(double) * 2);
	}
	//printf("transform splits file into %d slices\n", numBlocks);

	sf_count_t numRead;
	for(i = 0; i < numBlocks; i++){

		numRead = sf_readf_double( f, fftw_in, blocksize );

		if ( numRead < blocksize ) {
			//reached last block, which has less than blocksize frames
			//Pad with 0 so fft is same size as other blocks
			for( j = numRead; j < blocksize; j++ ) {
				fftw_in[j] = 0.0;
			}
		}

		fftw_execute( plan );

		for(j = 0; j < blocksize/2; j++){
			//printf("%ld\n", i*numBlocks + j);
			(*dft_data)[i*(blocksize/2) + j][0] = fftw_out[j][0];
			(*dft_data)[i*(blocksize/2) + j][1] = fftw_out[j][1];
		}
	}

	fftw_destroy_plan( plan );
	fftw_free( fftw_in );
	fftw_free( fftw_out);
	sf_close( f );

    return numBlocks * blocksize/2;
}