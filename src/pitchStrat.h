
/// @brief Callback function type "PitchStrategyFunc"
///
/// These callback functons are expected to identify pitches given the
/// spectrogram of an audio stream
/// 
/// @param[in] spectrogram A spectrogram produced for an audio stream. The
///            spectrogram should have been produced for `size/dftBlockSize`
///            blocks in the audio stream from a short-time fourier transform
///            and with the resulting spectrum being stored within different
///            rows of the array. There are `dftBlockSize` elements in each row
///            of the array (or block of data), which are contiguous in memory
///            and indicate the magnitude of the frequency bin.
/// @param[in] size The number of entries in spectrogram
/// @param[in] dftBlockSize The number of frequency bins in each block (row) of
///            the spectrogram. Note that this is different from `fftSize` (the
///            number of elements upon which the discrete fourier transform)
///            since negative frequencies are not included (The value of this
///            parameter is nominally `fftSize/2`) in the spectrogram.
///            that there are no negative frequency bins 
/// @param[in] hpsOvr Number of harmonic product specturm overtones to use with
///            the "hps" strategy. If an alternative strategy is in use, this
///            does nothing.
/// @param[in] fftSize The number of samples of the input audio used to compute
///            a single DFT.
/// @param[in] samplerate The sampling rate of the audio stream
/// @param[out] pitches A pre-allocate array of `size/dftBlocksize` entries
///             that will be filled with the frequencies of the identified
///             pitches
///
/// @return A value of 1 indicates success.
///
/// @par Note:
/// `fftSize` and `samplerate` are used to compute the frequency associated
/// with a given frequency bin. For frequency bin `i`, the associated frequency
/// is: `bin * (float)samplerate / fftSize`
///
/// @par
/// Based on the selected strategy, the spectrogram may actually be modified

typedef int (*PitchStrategyFunc)(float* spectrogram, int size,
				 int dftBlocksize, int hpsOvr,
				 int fftSize, int samplerate, float *pitches);
PitchStrategyFunc choosePitchStrategy(char* name);
int HPSDetectionStrategy(float* spectrogram, int size, int dftBlocksize,
			 int hpsOvr, int fftSize, int samplerate,
			 float* pitches);
int BaNaDetectionStrategy(float* spectrogram, int size, int dftBlocksize,
			  int hpsOvr, int fftSize, int samplerate,
			  float* pitches);
int BaNaMusicDetectionStrategy(float* spectrogram, int size,
			       int dftBlocksize, int hpsOvr, int fftSize,
			       int samplerate, float *pitches);
