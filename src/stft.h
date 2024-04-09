/// @file     stft.h
/// @brief    [\ref stft] Declaration of Short Time Fourier Transform functions

/// @defgroup stft Short Time Fourier Transform Module
///
/// In the future we may group this with other documentation

#include "melodyextraction.h"

/// @ingroup stft
/// Allocates a buffer and sets it to values appropriate for the Hamming Window 
float* WindowFunction(int size);

/// @ingroup stft
/// Determines the number of STFT blocks to be computed for an audio stream.
/// Each STFT block corresponds to a separate spectrum
///
/// @param[in] info Contains information about the audio stream
/// @param[in] unpaddedWinSize specifies the number of samples from the audio
///     stream that will be used to compute each FFT
/// @param[in] interval How much the window shifts between each FFT evaluation
int NumSTFTBlocks(audioInfo info, int unpaddedWinSize, int interval);

/// @ingroup stft
/// Computes a spectrogram by performing a short time fourier transform on an
/// audio stream (only the magnitude of the fourier spectra are returned)
///
/// In detail, this computes a STFT for an audio input via a sliding DFT. The
/// window includes `unpaddedWinSize` samples from the input stream and the
/// window is shifted by `interval` samples at a time. The total number of
/// window evaluations is given by the `NumSTFTBlocks` function. This
/// calculation employs a Hamming Window.
///
/// Prior to each DFT evaluation, the audio samples in the window are copied to
/// a buffer of `paddedFFTSize` elements. When `paddedFFTSize` exceeds
/// `unpaddedWinSize`, buffer is padded with zeros. This is a common
/// technique for increasing the frequency resolution of the output spectrum
/// (it's similar to interpolation).
///
/// The output data is a contiguous 2D array. The indices for the magnitudes of
/// the different frequency coefficients correspond to the fast indexing axis.
///
/// @param[in]  input Read-only array of audio inputs
/// @param[in]  info Contains information about the audio stream
/// @param[in]  unpaddedWinSize specifies the number of samples from the audio
///     stream that will be used to compute each FFT
/// @param[in]  paddedFFTSize the buffer size used for each FFT evaluation.
///     Must be no less than unpaddedWinSize
/// @param[in]  interval How much the window shifts between each FFT evaluation
/// @param[out] spectrogram This pointer is updated with a freshly allocated
///     buffer holding `NumSTFTBlocks` contiguous arrays. Each array holds the
///     magnitude of the coefficients measured at multiple frequencies.
///
/// @returns Upon success, this returns the number of complex numbers stored in
///     fftdata. Upon failure, this returns 0 or a negative number.
///
/// @note
/// Each of the output spectra omits the coefficient corresponding to the
/// Nyquist frequency. While this is unsurprising when `paddedFFTSize` is odd,
/// it may not be expected when `paddedFFTSize` is even. In the future, we may
/// want to revisit this behavior.
///
/// @todo
/// Document how the frequency of a given coefficient can be computed (it may
/// make more sense to direct people to a function like `BinToFreq`).
///
/// @todo
/// Document what the normalization (or lack thereof) actually is for the
/// output spectra. The FFTW3 manual calls the functions "unnormalized". This
/// means that performing an iFFT on the complex coefficients would return the
/// input signal multiplied by a constant.
int CalcSpectrogram(const float* input, audioInfo info, int unpaddedWinSize,
		    int paddedFFTSize, int interval, float** fft_data);
