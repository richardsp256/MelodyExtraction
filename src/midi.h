#include "melodyextraction.h"

/// converts a MIDI note number to a null-terminated string
/// @param[in]  n The input MIDI note number
/// @param[out] buffer Array of 5 elements where the name is written
///
/// @par Examples
/// To illustrate the usage some examples are provided:
/// - `n = 12` corresponds to `"C  1"`
/// - `n = 13` corresponds to `"C# 1"`
/// - `n = 123` corresponds to `"D#10"`
void NoteToName(int n, char* buffer);

/// Converts from frequency to the closest MIDI note
///
/// As an example, `FrequencyToNote(443.)` should give `57` (which corresponds
/// to A 4).
int FrequencyToNote(double freq);

/// Converts from frequency to the closest fractional MIDI note
float FrequencyToFractionalNote(double freq);


struct Midi* GenerateMIDIFromNotes(int* notePitches, int* noteRanges,
				   int nP_size, int sample_rate,
				   int *error_code);

/// Write Midi file header to file stream
/// @param[out] f File stream
/// @param[in]  format specifies file format
/// @param[in]  n_tracks Number of tracks in the file
/// @param[in]  division Specifies interpretation of event timings
///
/// @returns 0 upon success
int AddHeader(FILE* f, short format, short numTracks, short division);

/// Write Midi track to file stream
/// @param[out] f File stream
/// @param[in]  track Pointer to the sequence of MIDI events that is to be
///     written to the file
/// @param[in]  len The number of bytes in track
///
/// @returns 0 upon success
int AddTrack(FILE* f, const unsigned char* track, int len);

/// Write a 32-bit integer to an array of `unsigned char`
/// @param[out] c Array where the output is stored (must have 4 bytes of space)
/// @param[in]  num 32-bit integer value
void BigEndianInteger(unsigned char* c, int num);

/// Write a 16-bit integer to an array of `unsigned char`
/// @param[out] c Array where the output is stored (must have 2 bytes of space)
/// @param[in]  num 16-bit integer value
void BigEndianShort(unsigned char* c, short num);
