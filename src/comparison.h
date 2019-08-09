#include "fftw3.h"
#include "melodyextraction.h"


float* WindowFunction(int size);
struct Midi* ExtractMelody(float** input, audioInfo info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int o_unpaddedSize, int o_winSize, int o_winInt, OnsetStrategyFunc onsetStrategy,
		int s_winSize, int s_winInt, int s_mode, SilenceStrategyFunc silenceStrategy,
		int hpsOvr, int tuning, int verbose, char* prefix);
int ExtractPitch(float** input, float** pitches, audioInfo info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int hpsOvr, int verbose, char* prefix);
int ExtractSilence(float** input, int** activityRanges, audioInfo info,
		   int s_winSize, int s_winInt, int s_mode,
		   SilenceStrategyFunc silenceStrategy);
int ExtractOnset(float** input, int** onsets, audioInfo info, int o_unpaddedSize, int o_winSize, 
                  int o_winInt, OnsetStrategyFunc onsetStrategy, int verbose);
int ConstructNotes(int** noteRanges, float** noteFreq, float* pitches,
		   int p_size, int* onsets, int onset_size, int* activityRanges,
		   int aR_size, audioInfo info, int p_unpaddedSize, int p_winInt);
int FrequenciesToNotes(float* freq, int num_notes, int**melodyMidi, int tuning);
int STFT_r2c(float** input, audioInfo info, int unpaddedSize, int winSize, int interval, fftwf_complex** fft_data);
int STFTinverse_c2r(fftwf_complex** input, audioInfo info, int winSize, int interval, float** output);
float* Magnitude(fftwf_complex* arr, int size);
void SaveWeightsTxt(char* fileName, float** AudioData, int size, int dftBlocksize, int samplerate, int unpaddedSize, int winSize);
void SaveNotesTxt(char* fileName, int* noteRanges, int* notePitches,
		  int nP_size, int samplerate);
