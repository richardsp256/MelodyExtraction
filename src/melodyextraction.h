// Public header file
// also includes some Library-wide declarations

#ifndef MELODYEXTRACTION_H
#define MELODYEXTRACTION_H


#include <stdint.h> // for int64_t
#include "pitchStrat.h"
#include "onsetStrat.h"
#include "silenceStrat.h"

// the following is included so we can handle the basics of midi files

struct Track{
  unsigned char* data;
  int len;
};

struct Midi{
  struct Track** tracks;
  int format; //0 = single track, 1 = multitrack, 2 = multisong.
  int numTracks; //number of tracks
  int division; //delts time. if positive, represents units per beat. if negative, in SMPTE compatible units. 
};

struct Track* GenerateTrack(int* noteArr, int size, int verbose);
struct Midi* GenerateMIDI(int* noteArr, int size, int verbose);
void freeMidi(struct Midi* midi);
void SaveMIDI(struct Midi* midi, char* path, int verbose);

typedef struct {
	int64_t frames;
	int samplerate;
} audioInfo;

// like libfvad, libsamplerate, and fftw3, we define an object that we
// use to actually execute the library
// we may want to make this opaque

//struct me_settings;
struct me_settings{
	char * prefix;
	char * pitch_window;
	char * pitch_padded;
	char * pitch_spacing;
	char * pitch_strategy;
	char * onset_window;
	char * onset_padded;
	char * onset_spacing;
	char * onset_strategy;
	char * silence_window;
	char * silence_spacing;
	char * silence_strategy;
	int silence_mode;
	int hps;
	int tuning;
	int verbose;
};

struct me_data;

// create an instance of me_settings
struct me_settings* me_settings_new();

//destroy me setting
void me_settings_free(struct me_settings* inst);

// create an instance of me_data from me_settings
char* me_data_init(struct me_data** inst, struct me_settings* settings, audioInfo info);

// destroy me_data
void me_data_free(struct me_data *inst);

struct Midi* me_process(float **input, audioInfo info, struct me_data *inst);

#endif	/* MELODYEXTRACTION_H */
