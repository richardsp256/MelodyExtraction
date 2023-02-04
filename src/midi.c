#include <stdlib.h>
#include <stdio.h>
#include <math.h> // log2, round
#include <string.h>
#include <assert.h>
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

/// Write a 32-bit integer to filestream
///
/// @param[in] f filestream where the output is stored
/// @param[in] num 32-bit integer value
static int WriteBigEndianInteger(FILE* f, int32_t num){
	// convert to char[] so thats its Big-Endian
	unsigned char c[4];
	c[0] = (num >> 24) & 0xFF;
	c[1] = (num >> 16) & 0xFF;
	c[2] = (num >> 8) & 0xFF;
	c[3] = num & 0xFF;
	if (4 != fwrite(c, sizeof(unsigned char), 4, f)){
		return ME_ERROR;
	}
	return ME_SUCCESS;
}

/// Write a 16-bit integer to filestream
///
/// @param[in] f filestream where the output is stored
/// @param[in] num 16-bit integer value
static int WriteBigEndianShort(FILE* f, int16_t num){
	// convert to char[] so thats its Big-Endian
	unsigned char c[2];
	c[0] = (num >> 8) & 0xFF;
	c[1] = num & 0xFF;
	if (2 != fwrite(c, sizeof(unsigned char), 2, f)){
		return ME_ERROR;
	}
	return ME_SUCCESS;
}

/// Write Midi file header to file stream
/// @param[out] f File stream
/// @param[in]  format specifies file format. 0 = single track, 1 = multitrack,
///     2 = multisong.
/// @param[in]  n_tracks Number of tracks in the file
/// @param[in]  division Specifies interpretation of event timings. If
///     positive, represents units per beat. if negative, in SMPTE compatible
///     units.
///
/// @returns 0 upon success
static int AddHeader(FILE* f, short format, short numTracks, short division){
	// all midi file headers start fixed length string "MThd"
	if (4 != fwrite("MThd", sizeof(char), 4, f)){
		return ME_ERROR;
	}
	//MIDI files are big-endian, so reverse byte order of all ints & shorts
	int err_code;
	const int32_t length = 6; // length of the rest of the header, always 6
	if ((err_code = WriteBigEndianInteger(f, length)) != ME_SUCCESS){
		return err_code;
	}
	if ((err_code = WriteBigEndianShort(f, format)) != ME_SUCCESS){
		return err_code;
	}
	if ((err_code = WriteBigEndianShort(f, numTracks)) != ME_SUCCESS){
		return err_code;
	}
	if ((err_code = WriteBigEndianShort(f, division)) != ME_SUCCESS){
		return err_code;
	}

	return ME_SUCCESS;
}


/// @struct contextTStampToDeltaT
/// @brief Context object that is used when converting a sequence of time-stamps
///   to delta-times
///
/// Time-stamps are all measured relative to some absolute time. Delta-times
/// measures the time between time-stamps.
///
/// This supports conversions when the time-stamps are measured at a different
/// sampling rate from the delta-times.
typedef struct{
	uint64_t last_TStamp;
	double newSRate_div_oldSRate;
} contextTStampToDeltaT;

/// initialize context object for converting time-stamps to delta-times
///
/// the delta-time samplerate is the number of ticks per second in a Midi file
/// or: (dt_bpm * dt_division / 60 )
static contextTStampToDeltaT newContext_(int ts_samplerate,
					 int dt_bpm, int dt_division){
	contextTStampToDeltaT out
		= {0, (dt_bpm * dt_division)/(ts_samplerate * 60.0)};
	return out;
}

/// convert the time-stamp to a delta-time
static int DeltaTFromTStamp_(int time_stamp, contextTStampToDeltaT* context){
	assert(context->last_TStamp <= (uint64_t)time_stamp);

	// todo: clean this up (this looks a little weird right now to avoid
	// changing the outputs during refactoring)
	int delta_time_oldSRate = time_stamp - ((int)context->last_TStamp);
	context->last_TStamp = time_stamp;

	return (int)round(delta_time_oldSRate * context->newSRate_div_oldSRate);
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
static int UIntToVLQ(uint32_t num, unsigned char* out_buffer){
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


static MTrkEventBuffer MTrkEventNoteOn(uint32_t delta_time, int pitch,
				       int velocity) {
	MTrkEventBuffer out;
	int vlq_size = UIntToVLQ(delta_time, out.data);
	out.data[vlq_size+0] = (unsigned char)144; //NoteOn code for channel 1
	out.data[vlq_size+1] = (unsigned char)pitch;
	out.data[vlq_size+2] = (unsigned char)velocity;
	out.size = vlq_size + 3;
	return out;
}

static MTrkEventBuffer MTrkEventNoteOff(uint32_t delta_time, int pitch,
					int velocity) {
	MTrkEventBuffer out;
	int vlq_size = UIntToVLQ(delta_time, out.data);
	out.data[vlq_size+0] = (unsigned char)128; //NoteOff code for channel 1
	out.data[vlq_size+1] = (unsigned char)pitch;
	out.data[vlq_size+2] = (unsigned char)velocity;
	out.size = vlq_size + 3;
	return out;
}

static MTrkEventBuffer MTrkEventEndOfTrack(uint32_t delta_time){
	MTrkEventBuffer out;
	int vlq_size = UIntToVLQ(delta_time, out.data);
	out.data[vlq_size+0] = (unsigned char)0xFF;
	out.data[vlq_size+1] = (unsigned char)0x2F;
	out.data[vlq_size+2] = (unsigned char)0x00;
	out.size = vlq_size + 3;
	return out;
}

/// writes the notes to track the track chunk
///
/// @returns the number of bytes that were written upon success. Returns a
///    negative value on failure
static int WriteNotesToTrackChunk(FILE* f, int* notePitches,
				  int* noteRanges, int nP_size,
				  int bpm, int division, int sample_rate){

	const int vel = 80; // default velocity, given to all notes. This roughly
			    // corresponds to mezzo-forte dynamics (but the
			    // interpretation varies with software/hardware)

	int total_length = 0;

	// make preparations for converting time-stamps denoting the starts and
	// stops of notes (stored in noteRanges) to delta-times
	// - this conversion also converts from the input sample rate to units
	//   of ticks used in MIDI files (denoted by bpm and division)
	contextTStampToDeltaT context = newContext_(sample_rate, bpm, division);

	for (int i = 0; i < nP_size; ++i){

		int pitch = notePitches[i];
		for(int j = 0; j < 2; j++){
			uint32_t delta_time = DeltaTFromTStamp_
				(noteRanges[2*i+j], &context);

			MTrkEventBuffer b = (j == 0) ?
				MTrkEventNoteOn (delta_time, pitch, vel) :
				MTrkEventNoteOff(delta_time, pitch, vel);

			if (b.size != fwrite(b.data, sizeof(char), b.size, f)){
				return ME_ERROR;
			}
			total_length += b.size;
		}
	}

	MTrkEventBuffer b = MTrkEventEndOfTrack(2);
	if (b.size != fwrite(b.data, sizeof(char), b.size, f)){
		return ME_ERROR;
	}
	return total_length + b.size;
}

/// Write Midi track to file stream
///
/// @returns 0 upon success. negative values indicate failure
static int WriteMidiTrack(int* notePitches, int* noteRanges, int nP_size,
			  int bpm, int division, int sample_rate, FILE* f){
	// all track chunks start with fixed length string "MTrk"
	if (4 != fwrite("MTrk", sizeof(char), 4, f)){
		return ME_ERROR;
	}

	// record current location, where the length of the chunk is recorded
	long length_loc;
	if ((length_loc = ftell(f)) == -1L){ return ME_ERROR; }

	// provide a dummy value at current location since we don't know it yet
	int error_code;
	if ((error_code = WriteBigEndianInteger(f, 0)) != ME_SUCCESS){
		return error_code;
	}

	// write the rest of the track chunk
	int length = WriteNotesToTrackChunk(f, notePitches, noteRanges, nP_size,
					    bpm, division, sample_rate);
	if (length <= 0) {
		printf("track generation failed\n");
		return (length < 0) ? length : ME_ERROR;
	}

	// record current location for later
	long chunk_end_loc;
	if ((chunk_end_loc = ftell(f)) == -1L){ return ME_ERROR; }

	// now move file position indicator to record the length
	if (0 != fseek(f, length_loc, SEEK_SET)){
		printf("Error occured while shifting file position indicator "
		       "to fill in information about the track length\n");
		return ME_ERROR;
	}

	// actually record the length (this need to be encoded as big-endian)
	if ((error_code = WriteBigEndianInteger(f, length)) != ME_SUCCESS){
		return error_code;
	}

	// finally move file position indicator back to chunk_end_loc
	if (0 != fseek(f, chunk_end_loc, SEEK_SET)){
		printf("Error occured while shifting file position indicator "
		       "to end of track chunk after chunk length update\n");
		return ME_ERROR;
	}

	return ME_SUCCESS;
}

int WriteNotesAsMIDI(int* notePitches, int* noteRanges, int nP_size,
		     int sample_rate, FILE* f, int verbose)
{
	if (!f) { return ME_ERROR; }

	// first write the header
	const int bpm = 120;
	const int divisions = 48; //note: in the future dont hardcode
	const int midi_format = 1;
	const int num_tracks = 1;

	if (0 != AddHeader(f, midi_format, num_tracks, divisions)){
		fclose(f);
		return ME_ERROR;
	} else if (verbose){
		printf("header added\n");
		fflush(NULL);
	}

	// next write the track chunk
	int err_code = WriteMidiTrack(notePitches, noteRanges, nP_size,
				      bpm, divisions, sample_rate, f);
	if (err_code != ME_SUCCESS) {
		return err_code;
	} else if (verbose){
		printf("track added\n");
		fflush(NULL);
	}
	return ME_SUCCESS;
}


