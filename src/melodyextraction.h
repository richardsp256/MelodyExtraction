// Public header file
// also includes some Library-wide declarations

#ifndef MELODYEXTRACTION_H
#define MELODYEXTRACTION_H


#include <stdint.h> // for int64_t
#include <stdio.h> // for FILE
#include "pitch/pitchStrat.h"
#include "silenceStrat.h"

// the following is included so we can handle the basics of midi files

struct Track{
  unsigned char* data;
  int len;
};

struct Midi{
  struct Track* tracks;
  int format; //0 = single track, 1 = multitrack, 2 = multisong.
  int numTracks; //number of tracks
  int division; //delts time. if positive, represents units per beat. if negative, in SMPTE compatible units. 
};

void freeMidi(struct Midi* midi);

/// Writes midi data to a file
/// @param[in] midi pointer to the data that is to be saved
/// @param[in] f File stream where the midi will be written
/// @param[in] verbose Whether to run in verbose mode
///
/// @returns 0 upon success.
int SaveMIDI(struct Midi* midi, FILE* f, int verbose);

typedef struct {
	int64_t frames;
	int samplerate;
} audioInfo;

// like libfvad, libsamplerate, and fftw3, we define an object that we
// use to actually execute the library
// we may want to make this opaque

struct me_settings{
	char * prefix;
	char * pitch_window;
	char * pitch_padded;
	char * pitch_spacing;
	char * pitch_strategy;
	char * silence_window;
	char * silence_spacing;
	char * silence_strategy;
	int silence_mode;
	int hps;
	int tuning;
	int verbose;
	FILE *f;
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

/// process the input and write to the file pointer
///
/// @param[in]  input pointer to audio data that should be processed
/// @param[in]  info Information about the input audio data
/// @param[in]  me_data Holds settings information
/// @param[out] Optional pointer to an int where the exit code will be
///    recorded. A value of 0 indicates success. Pass the value to me_strerror
///    to retrieve the error message. This function will properly handle this
///    argument if it's NULL.
///
/// @returns 0 means success.
int me_process(float *input, audioInfo info, struct me_data *inst);

/// Convert an exit code to an string
///
/// @param error_code An error code returned from the
/// @returns This returns a string literal if the error code is recognzed
const char* me_strerror(int err_code);

#endif	/* MELODYEXTRACTION_H */
