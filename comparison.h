#include "sndfile.h"
#include "fftw3.h"
#include "pitchStrat.h"
#include "onsetStrat.h"
#include "silenceStrat.h"

void PrintAudioMetadata(SF_INFO * file);
float* WindowFunction(int size);
struct Midi* ExtractMelody(float** input, SF_INFO info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int o_unpaddedSize, int o_winSize, int o_winInt, OnsetStrategyFunc onsetStrategy,
		int s_winSize, int s_winInt, int s_mode, SilenceStrategyFunc silenceStrategy,
		int hpsOvr, int tuning, int verbose, char* prefix);
int ExtractPitch(float** input, float** pitches, SF_INFO info,
		int p_unpaddedSize, int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int hpsOvr, int verbose, char* prefix);
int ExtractSilence(float** input, int** activityRanges, SF_INFO info,
		   int s_winSize, int s_winInt, int s_mode,
		   SilenceStrategyFunc silenceStrategy);
int ExtractOnset(float** input, int** onsets, SF_INFO info, int o_unpaddedSize, int o_winSize, 
                  int o_winInt, OnsetStrategyFunc onsetStrategy, int verbose);
int ConstructNotes(int** noteRanges, float** noteFreq, float* pitches,
		   int p_size, int* onsets, int onset_size, int* activityRanges,
		   int aR_size, SF_INFO info, int p_unpaddedSize, int p_winInt);
void FrequenciesToNotes(float* freq, int num_notes, int**melodyMidi, int tuning);
int STFT_r2c(float** input, SF_INFO info, int unpaddedSize, int winSize, int interval, fftwf_complex** fft_data);
int STFTinverse_c2r(fftwf_complex** input, SF_INFO info, int winSize, int interval, float** output);
float* Magnitude(fftwf_complex* arr, int size);
void SaveAsWav(const double* audio, SF_INFO info, const char* path);
void SaveWeightsTxt(char* fileName, float** AudioData, int size, int dftBlocksize, int samplerate, int unpaddedSize, int winSize);
void SaveNotesTxt(char* fileName, int* noteRanges, int* notePitches,
		  int nP_size, int samplerate);