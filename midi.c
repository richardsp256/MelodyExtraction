#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "midi.h"

char* notes[] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
const int tuning = 440;

// converts from MIDI note number to string
// example: NoteToName(12)='C 1'
int isMidiNote(int x){
	return (x >= 0 && x <= 127);
}

void NoteToName(int n, char** name){
	//note: name should be allocated a length of 5
	if(!isMidiNote(n)){
		strcpy((*name), "----");
		return;
	}
	strcpy((*name), notes[n%12]);

	char* octave = malloc(sizeof(char)*3);
	sprintf(octave, "%d", n/12);
	strcat((*name), octave); /* add the extension */
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

void SaveMIDI(int* noteArr, int size, char* path, int verbose){
	//MIDI files are big-endian, so reverse byte order of all ints and shorts
	FILE* f = fopen(path, "wb+");

	AddHeader(&f, 0, 1, 24); //first 
	if(verbose){
		printf("header added\n");
		fflush(NULL);
	}

	unsigned char* trackData;
	int tracklength = MakeTrack(&trackData, noteArr, size);
	if(verbose){
		printf("track generated\n");
		fflush(NULL);
	}
	AddTrack(&f, trackData, tracklength);
	if(verbose){
		printf("track added\n");
		fflush(NULL);
	}

	fclose(f);
}

void AddHeader(FILE** f, short format, short tracks, short division){
	unsigned char* headerBuf = calloc(14, sizeof(char));

	char* descriptor = "MThd";
	memcpy( &headerBuf[0], &descriptor[0], 4 * sizeof(char) );

	unsigned char* tmp;

	AppendInt(&tmp, 6);
	memcpy( &headerBuf[4], &tmp[0], 4 * sizeof(char) );
	free(tmp);

	AppendShort(&tmp, format);
	memcpy( &headerBuf[8], &tmp[0], 2 * sizeof(char) );
	free(tmp);

	AppendShort(&tmp, tracks);
	memcpy( &headerBuf[10], &tmp[0], 2 * sizeof(char) );
	free(tmp);

	AppendShort(&tmp, division);
	memcpy( &headerBuf[12], &tmp[0], 2 * sizeof(char) );
	free(tmp);

	fwrite(headerBuf, sizeof(char), 14, (*f));
	free(headerBuf);
}

void AddTrack(FILE** f, unsigned char* track, int len){
	unsigned char* buf = calloc(len + 8, sizeof(char));

	char* descriptor = "MTrk";
	memcpy( &buf[0], &descriptor[0], 4 * sizeof(char) );

	unsigned char* tmp;
	AppendInt(&tmp, len);
	memcpy( &buf[4], &tmp[0], 4 * sizeof(char) );
	free(tmp);

	memcpy( &buf[8], &track[0], len * sizeof(char));

	fwrite(buf, sizeof(unsigned char), len + 8, (*f));
}

int MakeTrack(unsigned char** track, int* noteArr, int size){
	int dt = 2; //time between events.
	int timeSinceLast = 0;

	int vel = 80; //default velocity, given to all notes.
	int last = -1; //the last note that was played
	const unsigned char endTrack[3] = {(unsigned char) 255, (unsigned char) 47, (unsigned char) 0};

	unsigned char* timer;
	int timerSize = 0;
	unsigned char* message;

	(*track) = malloc(sizeof(char) * 10000); //experiment with size or more dynamic allocation
	int trackLen = 0; 


	for(int i = 0; i < size; ++i){
		timeSinceLast += dt;
		if(last == noteArr[i]  || !isMidiNote(noteArr[i])){
			continue;
		}

		timerSize = IntToVLQ(timeSinceLast, &timer);
		memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
		trackLen += timerSize;
		free(timer);

		if(last != -1){
			message = MessageNoteOff(last, vel);
			memcpy( &(*track)[trackLen], &message[0], 3 * sizeof(char));
			trackLen += 3;
			free(message);

			timerSize = IntToVLQ(0, &timer);
			memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
			trackLen += timerSize;
			free(timer);
		}

		message = MessageNoteOn(noteArr[i], vel);
		memcpy( &(*track)[trackLen], &message[0], 3 * sizeof(char));
		trackLen += 3;
		free(message);
		last = noteArr[i];
		timeSinceLast = 0;
	}

	timeSinceLast += dt;

	timerSize = IntToVLQ(timeSinceLast, &timer);
	memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
	trackLen += timerSize;
	free(timer);

	memcpy( &(*track)[trackLen], &endTrack[0], 3 * sizeof(char));
	trackLen += 3;

	return trackLen;
}

//todo: make single templated swapbytes function

void AppendInt(unsigned char** c, int num){
	//first convert to char[] so thats its Big-Endian
	(*c) = calloc(4, sizeof(char));
	(*c)[0] = (num >> 24) & 0xFF;
	(*c)[1] = (num >> 16) & 0xFF;
	(*c)[2] = (num >> 8) & 0xFF;
	(*c)[3] = num & 0xFF;
}

void AppendShort(unsigned char** c, short num){
	//first convert to char[] so thats its Big-Endian
	(*c) = calloc(2, sizeof(char));
	(*c)[0] = (num >> 8) & 0xFF;
	(*c)[1] = num & 0xFF;
}


//convert an integer to a VLQ (variable-length quantity)
int IntToVLQ(unsigned int num, unsigned char** VLQ){
	unsigned char buf[5] = ""; //no int will take more than 5
	int numBytes;
	int i;

	// convert to base 128
	//not that buf[0] contains 1's place, buf[1] contains 10's place, ect. 
	i = 0;
	do{
		buf[i] = (unsigned char)num % 128;
		num = num / 128;
		++i;
	}while (num != 0);

	numBytes = i; //number of bytes of the VLQ.

	(*VLQ) = malloc(sizeof(unsigned char) * numBytes);

	//reverse ordering of bytes, and set the leftmost bit of each byte.
	for(i = 0; i < numBytes; ++i){
		(*VLQ)[i] = buf[(numBytes-1) - i];
		//set leftmost bit to 1 for each byte other than 1's place
		if(i != (numBytes-1)){
			(*VLQ)[i] |= 1 << 7;
		}
	}

	return numBytes;
}

unsigned char* MessageNoteOn(int pitch, int velocity) {
	unsigned char* message = malloc(sizeof(unsigned char) * 3);
	message[0] = (unsigned char)144; //code for NoteOn event on channel 1
	message[1] = (unsigned char)pitch;
	message[2] = (unsigned char)velocity;
	return message;
}

unsigned char* MessageNoteOff(int pitch, int velocity) {
	unsigned char* message = malloc(sizeof(unsigned char) * 3);
	message[0] = (unsigned char)128; //code for NoteOn event on channel 1
	message[1] = (unsigned char)pitch;
	message[2] = (unsigned char)velocity;
	return message;
}