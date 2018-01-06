#include "melodyextraction.h"

int isMidiNote(int note);
void NoteToName(int n, char** name);
int FrequencyToNote(double freq);
float FrequencyToFractionalNote(double freq);
double log2(double x);
void AddHeader(FILE** f, short format, short tracks, short division);
void AddTrack(FILE** f, unsigned char* track, int len);
int MakeTrack(unsigned char** track, int trackCapacity, int* noteArr, int size);
struct Track* GenerateTrackFromNotes(int* notePitches, int* noteRanges,
				     int nP_size, int bpm, int division,
				     int sample_rate, int verbose);
struct Midi* GenerateMIDIFromNotes(int* notePitches, int* noteRanges,
				   int nP_size, int sample_rate,
				   int verbose);
int MakeTrackFromNotes(unsigned char** track, int trackCapacity,
		       int* notePitches, int* noteRanges, int nP_size,
		       int bpm, int division, int sample_rate,
		       int verbose);
void BigEndianInteger(unsigned char** c, int num);
void BigEndianShort(unsigned char** c, short num);
int IntToVLQ(unsigned int num, unsigned char** VLQ);
unsigned char* MessageNoteOn(int pitch, int velocity);
unsigned char* MessageNoteOff(int pitch, int velocity);
