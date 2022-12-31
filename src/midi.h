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


/// Writes note data to file in midi format
///
/// @returns 0 upon success.
int WriteNotesAsMIDI(int* notePitches, int* noteRanges, int nP_size,
		     int sample_rate, FILE* f, int verbose);

/// Write a 32-bit integer to an array of `unsigned char`
/// @param[out] c Array where the output is stored (must have 4 bytes of space)
/// @param[in]  num 32-bit integer value
void BigEndianInteger(unsigned char* c, int num);

/// Write a 16-bit integer to an array of `unsigned char`
/// @param[out] c Array where the output is stored (must have 2 bytes of space)
/// @param[in]  num 16-bit integer value
void BigEndianShort(unsigned char* c, short num);
