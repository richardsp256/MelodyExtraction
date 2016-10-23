#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "midi.h"

char* notes[] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
const int tuning = 440;
char* outFile = "output.mid";

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

void SaveMIDI(){
	//MIDI files are big-endian, so reverse byte order of all ints and shorts
	int deltaTime = 40;
	int vel = 60; //default velocity, given to all notes.
	int testNote = 41;
	FILE* f = fopen(outFile, "wb+");

	unsigned char* headerBuf = calloc(14, sizeof(char));
	int headerInd = 0;

	strncat(headerBuf, "MThd", 4);
	headerInd += 4;

	unsigned char* tmp = "";

	AppendInt(&tmp, 6);
	int p = (int)tmp[3];
	printf("head size:%d\n", p);
	memcpy( &headerBuf[headerInd], &tmp[0], 4 * sizeof(char) ); //note: maybe can just memcpy the int directly instead of converting to char* first...
	headerInd += 4;
	free(tmp);

	AppendShort(&tmp, 0);
	memcpy( &headerBuf[headerInd], &tmp[0], 2 * sizeof(char) );
	headerInd += 2;
	free(tmp);

	AppendShort(&tmp, 1);
	memcpy( &headerBuf[headerInd], &tmp[0], 2 * sizeof(char) );
	headerInd += 2;
	free(tmp);

	AppendShort(&tmp, 24);
	memcpy( &headerBuf[headerInd], &tmp[0], 2 * sizeof(char) );
	headerInd += 2;
	free(tmp);

	fwrite(headerBuf, sizeof(char), 14, f);

	printf("header added!\n");
	fflush(NULL);

	unsigned char* trackBuf;
	unsigned char trackDataBuf[1000] = ""; //change so you can expand buffer size
	int trackDataSize = 0; 
	unsigned char* timer;
	int timerSize = 0;
	unsigned char* message;

	timerSize = IntToVLQ(deltaTime, &timer);
	printf("timer1 bytes: %d\n", timerSize);
	memcpy( &trackDataBuf[trackDataSize], &timer[0], timerSize * sizeof(char));
	trackDataSize += timerSize;
	message = MessageNoteOn(testNote, vel);
	memcpy( &trackDataBuf[trackDataSize], &message[0], 3 * sizeof(char));
	trackDataSize += 3;

	free(timer);
	free(message);



	timerSize = IntToVLQ(deltaTime, &timer);
	printf("timer2 bytes: %d\n", timerSize);
	memcpy( &trackDataBuf[trackDataSize], &timer[0], timerSize * sizeof(char));
	trackDataSize += timerSize;
	message = MessageNoteOff(testNote, vel);
	memcpy( &trackDataBuf[trackDataSize], &message[0], 3 * sizeof(char));
	trackDataSize += 3;
	
	free(timer);
	free(message);



	timerSize = IntToVLQ(deltaTime, &timer);
	printf("timer3 bytes: %d\n", timerSize);
	memcpy( &trackDataBuf[trackDataSize], &timer[0], timerSize * sizeof(char));
	trackDataSize += timerSize;
	unsigned char endTrack[3] = {(unsigned char) 255, (unsigned char) 47, (unsigned char) 0};
	memcpy( &trackDataBuf[trackDataSize], &endTrack[0], 3 * sizeof(char));
	trackDataSize += 3;
	free(timer);



	printf("tracksize: %d\n", trackDataSize);

	trackBuf = calloc(trackDataSize + 8, sizeof(char));
	strncat(trackBuf, "MTrk", 4);

	AppendInt(&tmp, trackDataSize);
	memcpy( &trackBuf[4], &tmp[0], 4 * sizeof(char) );
	free(tmp);

	memcpy( &trackBuf[8], &trackDataBuf[0], trackDataSize * sizeof(char));

	fwrite(trackBuf, sizeof(unsigned char), trackDataSize + 8, f);
	
	printf("track added!\n");
	fflush(NULL);

	fclose(f);
}

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
