#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "midi.h"
#include "noteCompilation.h"

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
	return round(FrequencyToFractionalNote(freq));
}

float FrequencyToFractionalNote(double freq){
	return (12*log2(freq/tuning)) + 57;
}

double log2(double x){
	return log(x)/log(2);
}

struct Track* GenerateTrack(int* noteArr, int size, int verbose){
	struct Track *track;
	track = malloc(sizeof(struct Track));

	//MIDI files are big-endian, so reverse byte order of all ints and shorts
	int trackCapacity = 1000;
	unsigned char* trackData = malloc(sizeof(char) * trackCapacity);
	int tracklength = MakeTrack(&trackData, trackCapacity, noteArr, size);
	if(tracklength < 0){
		printf("track generation failed\n");
		free(trackData);
		return NULL;
	}

	track->len = tracklength;
	track->data = trackData;
	return track;
}

struct Midi* GenerateMIDI(int* noteArr, int size, int verbose){
	//MIDI files are big-endian, so reverse byte order of all ints and shorts
	struct Track* track;
	track = GenerateTrack(noteArr, size, verbose);
	if(!track){
		printf("track generation failed\n");
		return NULL;
	}
	else{
		struct Midi* midi;
		midi = malloc(sizeof(struct Midi));
		midi->tracks = malloc(sizeof(struct Track*) * 1);
		midi->tracks[0] = track;
		midi->format = 1;
		midi->numTracks = 1; //note: multiple tracks not currently supported
		midi->division = 24; //note: in the future dont hardcode
		return midi;
	}
}

void freeMidi(struct Midi* midi){
	for(int i = 0; i < midi->numTracks; ++i){
		struct Track* track = midi->tracks[i];
		free(track->data);
		free(track);
	}
	free(midi->tracks);
	free(midi);
}

void SaveMIDI(struct Midi* midi, char* path, int verbose){
	//MIDI files are big-endian, so reverse byte order of all ints and shorts
	FILE* f = fopen(path, "wb+");

	AddHeader(&f, midi->format, midi->numTracks, midi->division); //first 
	if(verbose){
		printf("header added\n");
		fflush(NULL);
	}
 
	for(int i = 0; i < midi->numTracks; ++i){
		struct Track* track = midi->tracks[i];
		AddTrack(&f, track->data, track->len);
		if(verbose){
			printf("track added\n");
			fflush(NULL);
		}
	}
	fclose(f);
}

void AddHeader(FILE** f, short format, short tracks, short division){
	unsigned char* headerBuf = calloc(14, sizeof(char));

	char* descriptor = "MThd"; //all midi headers start with MThd so it knows its a midi file
	memcpy( &headerBuf[0], &descriptor[0], 4 * sizeof(char) );

	unsigned char* tmp;

	BigEndianInteger(&tmp, 6); //length of the rest of the header, always 6
	memcpy( &headerBuf[4], &tmp[0], 4 * sizeof(char) );
	free(tmp);

	BigEndianShort(&tmp, format);
	memcpy( &headerBuf[8], &tmp[0], 2 * sizeof(char) );
	free(tmp);

	BigEndianShort(&tmp, tracks);
	memcpy( &headerBuf[10], &tmp[0], 2 * sizeof(char) );
	free(tmp);

	BigEndianShort(&tmp, division);
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
	BigEndianInteger(&tmp, len);
	memcpy( &buf[4], &tmp[0], 4 * sizeof(char) );
	free(tmp);

	memcpy( &buf[8], &track[0], len * sizeof(char));

	fwrite(buf, sizeof(unsigned char), len + 8, (*f));
}

int MakeTrack(unsigned char** track, int trackCapacity, int* noteArr, int size){
	int dt = 2; //time between events.
	int timeSinceLast = 0;

	int vel = 80; //default velocity, given to all notes.
	int last = -1; //the last note that was played

	unsigned char* timer;
	int timerSize = 0;
	unsigned char* message;

	int trackLen = 0; 


	for(int i = 0; i < size; ++i){
		timeSinceLast += dt;

		if(last == noteArr[i]  || !isMidiNote(noteArr[i])){
			continue;
		}

		if(last != -1){
			timerSize = IntToVLQ(timeSinceLast, &timer);
			message = MessageNoteOff(last, vel);

			if(trackLen + timerSize + 3 >= trackCapacity){
				//allocate more space for track
				trackCapacity += 1000;
				unsigned char* reallocatedTrack = realloc((*track), sizeof(char) * trackCapacity);
				if(reallocatedTrack){
					(*track) = reallocatedTrack;
				}
				else{ //memory could not be allocated
					free(timer);
					free(message);
					return -1;
				}
			}

			memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
			trackLen += timerSize;
			free(timer);
			memcpy( &(*track)[trackLen], &message[0], 3 * sizeof(char));
			trackLen += 3;
			free(message);
			timeSinceLast = 0;
		}

		timerSize = IntToVLQ(timeSinceLast, &timer);
		message = MessageNoteOn(noteArr[i], vel);

		if(trackLen + timerSize + 3 >= trackCapacity){
			//allocate more space for track
			trackCapacity += 1000;
			unsigned char* reallocatedTrack = realloc((*track), sizeof(char) * trackCapacity);
			if(reallocatedTrack){
				(*track) = reallocatedTrack;
			}
			else{ //memory could not be allocated
				free(timer);
				free(message);
				return -1;
			}
		}

		memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
		trackLen += timerSize;
		free(timer);
		memcpy( &(*track)[trackLen], &message[0], 3 * sizeof(char));
		trackLen += 3;
		free(message);
		timeSinceLast = 0;
		last = noteArr[i];
	}

	timeSinceLast += dt;

	timerSize = IntToVLQ(timeSinceLast, &timer);
	unsigned char endTrack[3] = {(unsigned char) 255, (unsigned char) 47, (unsigned char) 0}; //end track value defined in MIDI standard

	if(trackLen + timerSize + 3 >= trackCapacity){
		//allocate more space for track
		trackCapacity += 1000;
		unsigned char* reallocatedTrack = realloc((*track), sizeof(char) * trackCapacity);
		if(reallocatedTrack){
			(*track) = reallocatedTrack;
		}
		else{ //memory could not be allocated
			free(timer);
			return -1;
		}
	}

	memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
	trackLen += timerSize;
	free(timer);
	memcpy( &(*track)[trackLen], &endTrack[0], 3 * sizeof(char));
	trackLen += 3;

	return trackLen;
}

struct Track* GenerateTrackFromNotes(int* notePitches, int* noteRanges,
				     int nP_size, int bpm, int division,
				     int sample_rate, int verbose){
	struct Track *track;
	track = malloc(sizeof(struct Track));

	//MIDI files are big-endian, so reverse byte order of all ints and shorts
	int trackCapacity = 1000;
	unsigned char* trackData = malloc(sizeof(char) * trackCapacity);
	int tracklength = MakeTrackFromNotes(&trackData, trackCapacity, notePitches,
					     noteRanges, nP_size, bpm, division,
					     sample_rate, verbose);
	if(tracklength < 0){
		printf("track generation failed\n");
		free(trackData);
		free(track);
		return NULL;
	}

	track->len = tracklength;
	track->data = trackData;
	return track;
}

struct Midi* GenerateMIDIFromNotes(int* notePitches, int* noteRanges,
				   int nP_size, int sample_rate,
				   int verbose)
{
	//MIDI files are big-endian, so reverse byte order of all ints and shorts
	struct Track* track;
	int bpm = 120;
	int divisions = 48;
	track = GenerateTrackFromNotes(notePitches, noteRanges, nP_size, bpm,
				       divisions, sample_rate, verbose);
	if(!track){
		return NULL;
	}
	else{
		struct Midi* midi;
		midi = malloc(sizeof(struct Midi));
		midi->tracks = malloc(sizeof(struct Track*) * 1);
		midi->tracks[0] = track;
		midi->format = 1;
		midi->numTracks = 1; //note: multiple tracks not currently supported
		midi->division = divisions; //note: in the future dont hardcode
		return midi;
	}
}



int MakeTrackFromNotes(unsigned char** track, int trackCapacity,
		       int* notePitches, int* noteRanges, int nP_size,
		       int bpm, int division, int sample_rate,
		       int verbose){
	int timeSinceLast = 0;

	int vel = 80; //default velocity, given to all notes.

	unsigned char* timer;
	int timerSize = 0;
	unsigned char* message;

	int trackLen = 0; 
	int* eventRanges = noteRangesEventTiming(noteRanges, 2*nP_size,
						 sample_rate, bpm, division);
	
	for(int i = 0; i < nP_size; ++i){
		// start the note
		timeSinceLast = eventRanges[2*i];
		timerSize = IntToVLQ(timeSinceLast, &timer);
		message = MessageNoteOn(notePitches[i], vel);

		if(trackLen + timerSize + 3 >= trackCapacity){
			//allocate more space for track
			trackCapacity += 1000;
			unsigned char* reallocatedTrack = realloc((*track), sizeof(char) * trackCapacity);
			if(reallocatedTrack){
				(*track) = reallocatedTrack;
			}
			else{ //memory could not be allocated
				free(timer);
				free(message);
				return -1;
			}
		}

		memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
		trackLen += timerSize;
		free(timer);
		memcpy( &(*track)[trackLen], &message[0], 3 * sizeof(char));
		trackLen += 3;
		free(message);

		// end the note

		timeSinceLast = eventRanges[2*i+1];
		timerSize = IntToVLQ(timeSinceLast, &timer);
		message = MessageNoteOff(notePitches[i], vel);

		if(trackLen + timerSize + 3 >= trackCapacity){
			//allocate more space for track
			trackCapacity += 1000;
			unsigned char* reallocatedTrack = realloc((*track), sizeof(char) * trackCapacity);
			if(reallocatedTrack){
				(*track) = reallocatedTrack;
			}
			else{ //memory could not be allocated
				free(timer);
				free(message);
				return -1;
			}
		}

		memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
		trackLen += timerSize;
		free(timer);
		memcpy( &(*track)[trackLen], &message[0], 3 * sizeof(char));
		trackLen += 3;
		free(message);
	}


	free(eventRanges);
	
	timeSinceLast = 2;

	timerSize = IntToVLQ(timeSinceLast, &timer);
	unsigned char endTrack[3] = {(unsigned char) 255, (unsigned char) 47, (unsigned char) 0}; //end track value defined in MIDI standard

	if(trackLen + timerSize + 3 >= trackCapacity){
		//allocate more space for track
		trackCapacity += 1000;
		unsigned char* reallocatedTrack = realloc((*track), sizeof(char) * trackCapacity);
		if(reallocatedTrack){
			(*track) = reallocatedTrack;
		}
		else{ //memory could not be allocated
			free(timer);
			return -1;
		}
	}

	memcpy( &(*track)[trackLen], &timer[0], timerSize * sizeof(char));
	trackLen += timerSize;
	free(timer);
	memcpy( &(*track)[trackLen], &endTrack[0], 3 * sizeof(char));
	trackLen += 3;

	return trackLen;
}

//todo: make single templated swapbytes function

void BigEndianInteger(unsigned char** c, int num){
	//first convert to char[] so thats its Big-Endian
	(*c) = calloc(4, sizeof(char));
	(*c)[0] = (num >> 24) & 0xFF;
	(*c)[1] = (num >> 16) & 0xFF;
	(*c)[2] = (num >> 8) & 0xFF;
	(*c)[3] = num & 0xFF;
}

void BigEndianShort(unsigned char** c, short num){
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
	message[0] = (unsigned char)128; //code for NoteOff event on channel 1
	message[1] = (unsigned char)pitch;
	message[2] = (unsigned char)velocity;
	return message;
}
