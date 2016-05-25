#include <stdio.h>
#include <stdlib.h>
#include "comparison.h"

int main(int argc, char ** argv)
{
	int blocksize = 2048;

	//check command line arguments
	if(argc != 2){
		printf("ERROR: incorrect command line args. Usage is as follows:\
			\n  [1] Input .wav file\n");
		return -1;
	}
	char* fname = argv[1];

	//run STFT on input
    double** AudioData = NULL;
    unsigned int samplerate = 0;
    unsigned int frames = 0;
    int size = ReadAudioFile(fname, blocksize, &AudioData, &samplerate, &frames);
	if( !size ) {
		printf("ERROR: ReadAudioFile failed for %s\n", fname);
	}
	printf("size is %d\n", size);
	printf("samplerate is %d\n", samplerate);
	printf("frames is %d\n", frames);

	//free data
	int i;
	for(i = 0; i < size; i++){
		free(AudioData[i]);
	}
	free(AudioData);

	return 0;
}