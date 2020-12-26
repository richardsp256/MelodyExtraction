/// @file     BaNaDetection.h
/// @brief    Declaration of interface function used for BaNa pitch detection

#include <stdbool.h>
#include "../lists.h"

// returns 1 on success
int BaNa(float **AudioData, int size, int dftBlocksize, int p,
	 float f0Min, float f0Max, float xi, int fftSize, int samplerate,
	 bool first, float* fundamentals);
float* calcFrequencies(int dftBlocksize, int fftSize, int samplerate);

/// Iterates over each spectrum in the spectrogram and sets the magnitude for
/// all frequencies outside of [f0Min,p*f0Max] to zero
///
/// @param[in, out] spectrogram This should hold `size/dftBlockSize` spectra.
///                 There are `dftBlockSize` frequency bins in each row, which
///                 are contiguous in memory and indicate the magnitude of the
///                 basis function in that bin.
/// @param[in]      size The total number of elements in the spectrogram
/// @param[in]      dftBlockSize The number of frequency bins in each spectrum
/// @param[in]      p the number of harmonic peaks to search for.
/// @param[in]      f0Min The minimum allowed fundamental frequency that can be
///                 returned by the pitch detection algorithm
/// @param[in]      f0Max The maximum allowed fundamental frequency that can be
///                 returned by the pitch detection algorithm.
/// @param[in]      frequencies An array holding the central frequency of every
///                 spectrum bin.
///
/// @note
/// It's unclear exactly how necessary this step is, especially if a bandpass
/// filter is applied to the audio before computing the spectrogram.
/// At the very least, it seems like bad practice to modify the spectrogram
/// directly.
void BaNaPreprocessing(float *spectrogram, int size, int dftBlocksize, int p,
		       float f0Min, float f0Max, const float* frequencies);

/// Identifies the candidates for the fundamental frequencies
///
/// @param[in]  spectrogram This should hold `size/dftBlockSize` spectra. There
///             are `dftBlockSize` frequency bins in each row, which are
///             contiguous in memory and indicate the magnitude of the basis
///             function in that bin.
/// @param[in]  size The total number of elements in the spectrogram
/// @param[in]  dftBlockSize The number of frequency bins in each spectrum
/// @param[in]  p the number of harmonic peaks to search for.
/// @param[in]  f0Min The minimum allowed fundamental frequency that can be
///             returned by the pitch detection algorithm
/// @param[in]  f0Max The maximum allowed fundamental frequency that can be
///             returned by the pitch detection algorithm.
/// @param[in]  first When this is true, the lowest magnitude peaks are treated
///             as candidates. When False, the highest magnitude peaks are
///             treated as candidates.
/// @param[in]  xi The tolerance around the harmonics that is used for picking
///             candidates (I think)
/// @param[in]  frequencies An array holding the central frequency of every
///             spectrum bin.
/// @param[in]  fftSize The number of samples of the input audio used to
///             compute a single DFT.
/// @param[in]  samplerate The sampling rate of the audio stream
/// @param[out] windowCandidates This is a list of pointers. This should have
///             `(size / dftBlocksize)` entries. Each entry in the list will be
///             overwritten with a distinctList holding all all candidates for
///             that frame.
/// @returns 0 indicates success
int BaNaFindCandidates(const float *spectrogram, int size, int dftBlocksize,
		       int p, float f0Min, float f0Max, bool first, float xi,
		       float* frequencies, int fftSize, int samplerate,
		       distinctList** windowCandidates);
