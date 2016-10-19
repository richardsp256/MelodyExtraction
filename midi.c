#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "midi.h"

char* notes[] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
const int tuning = 440;

// converts from MIDI note number to string
// example: NoteToName(12)='C 1'
void NoteToName(int n, char** name){
	if(n < 0 || n > 119){
		strcpy((*name), "---");
		return;
	}
	strcpy((*name), notes[n%12]);
	printf("wwW");
	fflush(NULL);

	char* octave = malloc(sizeof(char)*2);
	sprintf(octave, "%d", n/12);
	printf("rrr");
	fflush(NULL);
	strcat((*name), octave); /* add the extension */
	printf("qqq");
	fflush(NULL);
	free(octave);
	return;
}

// converts from frequency to closest MIDI note
// example: FrequencyToNote(443)=57 (A 4)
int FrequencyToNote(double freq){
	return round(12*log2(freq/tuning)) + 57;
}

double log2(double x){
	return log(x)/log(2);
}