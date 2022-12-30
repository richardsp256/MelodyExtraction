#include <stdlib.h>
#include <stdio.h>
#include <math.h> // log2, round
#include <string.h>
#include "midi.h"
#include "noteCompilation.h"
#include "errors.h"

static const char notes[12][3] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
static const int tuning = 440;

static int isMidiNote(int x){
	return (x >= 0 && x <= 127);
}

void NoteToName(int n, char* name){
	// name is assumed to have a length of 5
	if(isMidiNote(n)){
		snprintf(name, 5, "%s%2d", notes[n%12], n/12);
	}else{
		strcpy(name, "----");
	}
}

// converts from frequency to closest MIDI note
// example: FrequencyToNote(443)=57 (A 4)
int FrequencyToNote(double freq){
	return round(FrequencyToFractionalNote(freq));
}

float FrequencyToFractionalNote(double freq){
	float fractNote = (12*log2(freq/tuning)) + 57;
	return fractNote;
}



void freeMidi(struct Midi* midi){
	for(int i = 0; i < midi->numTracks; ++i){
		struct Track* track = &(midi->tracks[i]);
		free(track->data);
	}
	free(midi->tracks);
	free(midi);
}

int SaveMIDI(struct Midi* midi, const char* path, int verbose){
	//MIDI files are big-endian, so reverse byte order of all ints and shorts
	FILE* f = fopen(path, "wb+");
	if(!f) {
		return -2;
	}

	if (0 != AddHeader(f, midi->format, midi->numTracks, midi->division)){
		fclose(f);
		return -1;
	} else if (verbose){
		printf("header added\n");
		fflush(NULL);
	}
 
	for(int i = 0; i < midi->numTracks; ++i){
		struct Track* track = &(midi->tracks[i]);
		if (0 != AddTrack(f, track->data, track->len)){
			fclose(f);
			return -1;
		} else if(verbose){
			printf("track added\n");
			fflush(NULL);
		}
	}
	fclose(f);
	return 0;
}

int AddHeader(FILE* f, short format, short numTracks, short division){
	unsigned char headerBuf[14]; // entries automatically set to 0

	// all midi file headers start fixed length string "MThd"
	memcpy( &headerBuf[0], "MThd", 4 * sizeof(char) );
	BigEndianInteger(&headerBuf[4], 6); // length of the rest of the header, always 6
	BigEndianShort(&headerBuf[8], format);
	BigEndianShort(&headerBuf[10], numTracks);
	BigEndianShort(&headerBuf[12], division);

	if (14 != fwrite(headerBuf, sizeof(unsigned char), 14, f)){
		return -1;
	}
	return 0;
}

int AddTrack(FILE* f, const unsigned char* track, int len){
	// first, write the track header
	unsigned char trackHeader[8]; // entries automatically set to 0
	memcpy( &trackHeader[0], "MTrk", 4 * sizeof(char) );
	BigEndianInteger(&trackHeader[4], len);
	if (8 != fwrite(&trackHeader[0], sizeof(char), 8, f)){
		return -1;
	}

	// second, write the track data
	if (len != fwrite(track, sizeof(unsigned char), len, f)){
		return -1;
	}

	return 0;
}

typedef struct{unsigned char* buf; size_t length; size_t capacity;} growable_buf;

static int buf_append_(unsigned char* item, size_t item_size, growable_buf* b){
	const size_t capacity_increment = 100;

	// ok to call realloc with b->buf is NULL, but enforce length & capacity
	if (b->buf == NULL){
		b->length = 0;
		b->capacity = 0;
	}

	if ((item_size + b->length) > b->capacity){ // allocate more space
		size_t new_cap = b->capacity + capacity_increment;
		unsigned char* tmp = realloc(b->buf,
					     new_cap * sizeof(unsigned char));
		if (tmp == NULL){ return ME_REALLOC_FAILURE; }
		b->buf = tmp;
		b->capacity = new_cap;
	}

	memcpy(b->buf + b->length, item, item_size);
	b->length += item_size;

	return ME_SUCCESS;
}

// This specifies the max number of bytes in a variable length quantity. For
// reference, each byte in a variable length quantity only uses 7 bits to
// encode a value.
// - the MIDI specification seems to hint that you should only encode a 32bit
//   unsigned integer.
#define MAX_BYTES_VLQ 5             /* = ceil(sizeof(uint32_t) * 8.0 / 7.0) */

/// converts a uint32_t to a VLQ (variable-length quantity)
///
/// @param[in] num the value to encode
/// @param[in] out_buffer the array where the data will be written. This
///     should be pre-allocationed and provide space for MAX_BYTES_VLQ values
int UIntToVLQ(uint32_t num, unsigned char* out_buffer){
	unsigned char buf[MAX_BYTES_VLQ] = "";

	// convert to base 128
	// note that buf[0] contains 1's place, buf[1] contains 10's place, etc
	int byte_count = 0;
	do{
		buf[byte_count] = (unsigned char)num % 128;
		num = num / 128;
		++byte_count;
	}while (num != 0);

	//reverse ordering of bytes, and set the leftmost bit of each byte.
	for(int i = 0; i < byte_count; ++i){
		out_buffer[i] = buf[(byte_count-1) - i];
		//set leftmost bit to 1 for each byte other than 1's place
		if(i != (byte_count-1)){
			out_buffer[i] |= 1 << 7;
		}
	}

	return byte_count;
}

unsigned char* MessageNoteOn(int pitch, int velocity) {
	unsigned char* message = malloc(sizeof(unsigned char) * 3);
	message[0] = (unsigned char)144; //code for NoteOn event on channel 1
	message[1] = (unsigned char)pitch;
	message[2] = (unsigned char)velocity;
	return message;
}

/// @struct MTrkEventBuffer
/// @brief Encodes all data in a <MTrk event>
///
/// According to the MIDI spec, the data in a MIDI Track chunk (a MTrK chunk),
/// following the header is composed of a sequence of <MTrK event>s.
///
/// Each <MTrK event> consists of: <delta-time> <event>
/// 1. <delta-time> is a variable length quantity that stores the number of
///    ticks between the preceeding <MTrk event> (or the start of the track if
///    there isn't a preceeding one) and the following <event>
/// 2. <event> encodes event-data. Currently, we just support Note-On, Note-Off,
///    and End-of-Track. To avoid confusion, we will sometimes call this a
///    message.
///
/// The two parts are not differentiated in this struct. The maximum supported
/// message size is currently 3 bytes.
typedef struct{
	unsigned char data[MAX_BYTES_VLQ + 3];
	int size; // specifies how much of data is filled
} MTrkEventBuffer;


MTrkEventBuffer MTrkEventNoteOn(uint32_t delta_time, int pitch, int velocity) {
	MTrkEventBuffer out;
	int vlq_size = UIntToVLQ(delta_time, out.data);
	out.data[vlq_size+0] = (unsigned char)144; //NoteOn code for channel 1
	out.data[vlq_size+1] = (unsigned char)pitch;
	out.data[vlq_size+2] = (unsigned char)velocity;
	out.size = vlq_size + 3;
	return out;
}

MTrkEventBuffer MTrkEventNoteOff(uint32_t delta_time, int pitch, int velocity) {
	MTrkEventBuffer out;
	int vlq_size = UIntToVLQ(delta_time, out.data);
	out.data[vlq_size+0] = (unsigned char)128; //NoteOff code for channel 1
	out.data[vlq_size+1] = (unsigned char)pitch;
	out.data[vlq_size+2] = (unsigned char)velocity;
	out.size = vlq_size + 3;
	return out;
}

MTrkEventBuffer MTrkEventEndOfTrack(uint32_t delta_time){
	MTrkEventBuffer out;
	int vlq_size = UIntToVLQ(delta_time, out.data);
	out.data[vlq_size+0] = (unsigned char)0xFF;
	out.data[vlq_size+1] = (unsigned char)0x2F;
	out.data[vlq_size+2] = (unsigned char)0x00;
	out.size = vlq_size + 3;
	return out;
}

int MakeTrackFromNotes(growable_buf* trackBuf, int* notePitches,
		       int* noteRanges, int nP_size,
		       int bpm, int division, int sample_rate){

	const int vel = 80; // default velocity, given to all notes. This roughly
			    // corresponds to mezzo-forte dynamics (but the
			    // interpretation varies with software/hardware)

	int* eventRanges = noteRangesEventTiming(noteRanges, 2*nP_size,
						 sample_rate, bpm, division);
	if (eventRanges == NULL){ return ME_MALLOC_FAILURE; }

	for(int i = 0; i < nP_size; ++i){
		for(int j = 0; j < 2; j++){
			uint32_t delta_time = (uint32_t)eventRanges[2*i + j];
			int pitch = notePitches[i];

			MTrkEventBuffer entry_buf = (j == 0) ?
				MTrkEventNoteOn (delta_time, pitch, vel) :
				MTrkEventNoteOff(delta_time, pitch, vel);

			int rcode = buf_append_(entry_buf.data, entry_buf.size,
						trackBuf);
			if (rcode != ME_SUCCESS){
				free(eventRanges);
				return rcode;
			}
		}
	}
	free(eventRanges);

	MTrkEventBuffer entry_buf = MTrkEventEndOfTrack(2);
	return buf_append_(entry_buf.data, entry_buf.size, trackBuf);
}

static struct Track GenerateTrackFromNotes(int* notePitches, int* noteRanges,
					   int nP_size, int bpm, int division,
					   int sample_rate, int* error_code){
	struct Track track = {.len = 0, .data = NULL};

	growable_buf trackBuf = {.buf=NULL, .length=0, .capacity=0};
	*error_code = MakeTrackFromNotes(&trackBuf, notePitches, noteRanges,
					 nP_size, bpm, division, sample_rate);
	if((*error_code) != ME_SUCCESS){
		printf("track generation failed\n");
		if (trackBuf.buf){ free(trackBuf.buf); }
		return track;
	}

	track.len = trackBuf.length;
	track.data = trackBuf.buf;
	return track;
}

struct Midi* GenerateMIDIFromNotes(int* notePitches, int* noteRanges,
				   int nP_size, int sample_rate,
				   int* error_code)
{
	if (error_code == NULL){ return NULL; }

	struct Track* track_ptr = malloc(sizeof(struct Track));
	if (track_ptr == NULL){
		*error_code = ME_MALLOC_FAILURE;
		return NULL;
	}
	int bpm = 120;
	int divisions = 48;
	*track_ptr = GenerateTrackFromNotes(notePitches, noteRanges, nP_size,
					    bpm, divisions, sample_rate,
					    error_code);
	if(*error_code != ME_SUCCESS){
		free(track_ptr);
		return NULL;
	}

	struct Midi* midi;
	midi = malloc(sizeof(struct Midi));
	if (!midi){
		free(track_ptr->data);
		free(track_ptr);
		*error_code = ME_MALLOC_FAILURE;
		return NULL;
	}

	midi->tracks = track_ptr;
	midi->format = 1;
	midi->numTracks = 1; //note: multiple tracks not currently supported
	midi->division = divisions; //note: in the future dont hardcode
	return midi;
}

void BigEndianInteger(unsigned char* c, int num){
	// convert to char[] so thats its Big-Endian
	c[0] = (num >> 24) & 0xFF;
	c[1] = (num >> 16) & 0xFF;
	c[2] = (num >> 8) & 0xFF;
	c[3] = num & 0xFF;
}

void BigEndianShort(unsigned char* c, short num){
	// convert to char[] so thats its Big-Endian
	c[0] = (num >> 8) & 0xFF;
	c[1] = num & 0xFF;
}





