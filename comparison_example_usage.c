#include <stdio.h>
#include <stdlib.h>
#include "comparison.h"

int main(int argc, char ** argv)
{
	//this function provides an example of how this code should be utilized.
	//goalFile is our original audio file.
	//the tests are all hypothetical individuals to evaluate the fitness of.
	char* goalFile = "./test.wav";

	//Code to read in the original audio file that the gen alg is trying to replicate. should only be read in once, and FFT data passed to where it is needed
    double** Goal = NULL;
    unsigned int samplerate = 0;
    unsigned int frames = 0;
    int size = ReadAudioFile(goalFile, &Goal, &samplerate, &frames);
	if( !size ) {
		printf("error: ReadAudioFile failed for %s\n", goalFile);
	}
	printf("samplerate is %d\n", samplerate);
	printf("frames is %d\n", frames);

	//Code to test fitness of individual by comparing individuals FFT to the Original FFT.
	//takes in Sample* as first arg, which is audio data generated from audio.c
	/*
	Sample* samples = NULL; //would be set in audio.c
	int numSamples = 0;
	double fitness = AudioComparison(samples, numSamples, Goal, size);
	printf("%s has a fitness of %f\n", test1, fitness);
	*/

	//free data
	int i;
	for(i = 0; i < size; i++){
		free(Goal[i]);
	}
	free(Goal);

	return 0;
}