#include "melodyextraction.h"
#include "lists.h"


struct Midi* ExtractMelody(float** input, audioInfo info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int o_unpaddedSize, int o_winSize, int o_winInt, OnsetStrategyFunc onsetStrategy,
		int s_winSize, int s_winInt, int s_mode, SilenceStrategyFunc silenceStrategy,
		int hpsOvr, int tuning, int verbose, char* prefix,
		int *exit_code);

/// Extracts the pitches from audio
///
/// This function performs a series of short-time fourier transforms on the
/// audio and then passes the result to a specified pitch strategy.
///
/// @param[in] input The input audio data to extract the pitches from
/// @param[out] pitches An array where the identified pitches are stored. This
///             array should be pre-allocated and has a length given by
///             `NumSTFTBlocks(info, p_unpaddedSize, p_winInt)`. The length of
///             the array is also returned by the function
/// @param[in] info Holds information about the audio data
/// @param[in] p_unpaddedSize The number of elements from `input` included in
///            the window used for each DFT
/// @param[in] p_winSize The size of the window used for each DFT. This must be
///            at least as large as `p_unpaddedSize`. When
///            `p_winSize > p_unpaddedSize`, the window is first filled with
///            entries from `input` and then the remaining space is padded with
///            zeros. (Zero-padding is used to control the size of the
///            frequency bins in the resulting spectrum).
/// @param[in] p_winInt The interval between the first entry in `input`
///            included in each window. This is typically less than the value
///            p_unpaddedSize.
/// @param[in] pitchStrategy The callback function actually used to extract the
///            pitches
/// @param[in] hpsOvr Number of harmonic product specturm overtones to use with
///            the "hps" strategy. If an alternative strategy is in use, this
///            does nothing.
/// @param[in] verbose Controls the verbosity of the functions. A value of 1
///            indicates that the messages should be verbose
/// @param[in] prefix A string prefix indicating where spectrograms used for
///            debugging should be saved. The spectrogram produced directly
///            from the audio will be saved at "{prefix}_original.txt" and a
///            spectrogram that may or may not have been modified (weighted) by
///            the pitch detection strategy will be saved at
///            "{prefix}_weighted.txt".
///
/// @return Returns the length of pitches. If the value is not positive, then
///         an error occured
///
/// @par Note:
/// If we ever use a strategy where the number of output pitches is not
/// definitively known before calling this function, we can easily implement a
/// resizable floatList
int ExtractPitch(float* input, float* pitches, audioInfo info,
		 int p_unpaddedSize, int p_winSize, int p_winInt,
		 PitchStrategyFunc pitchStrategy, int hpsOvr, int verbose,
		 char* prefix);

/// Extracts pitches from audio and allocates the memory to hold the data
///
/// Wraps the ExtractPitch function
///
/// @return Returns the length of pitches. If the value is not positive, then
///         an error occured
int ExtractPitchAndAllocate(float** input, float** pitches, audioInfo info,
			    int p_unpaddedSize, int p_winSize, int p_winInt,
			    PitchStrategyFunc pitchStrategy, int hpsOvr,
			    int verbose, char* prefix);
int ExtractSilence(float** input, int** activityRanges, audioInfo info,
		   int s_winSize, int s_winInt, int s_mode,
		   SilenceStrategyFunc silenceStrategy);
int ExtractOnset(float** input, intList* onsets, audioInfo info, int o_unpaddedSize, int o_winSize, 
                  int o_winInt, OnsetStrategyFunc onsetStrategy, int verbose);
int ConstructNotes(int** noteRanges, float** noteFreq, float* pitches,
		   int p_size, intList* onsets, int onset_size,
		   int* activityRanges, int aR_size, audioInfo info,
		   int p_unpaddedSize, int p_winInt);
int FrequenciesToNotes(float* freq, int num_notes, int**melodyMidi, int tuning);

void PrintDetectionSummary(audioInfo info, const int * noteRanges,
			   const float * noteFreq, const int * melodyMidi,
			   int num_notes);
void SaveWeightsTxt(char* fileName, float** AudioData, int size, int dftBlocksize, int samplerate, int unpaddedSize, int winSize);
void SaveNotesTxt(char* fileName, int* noteRanges, int* notePitches,
		  int nP_size, int samplerate);
