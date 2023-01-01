#include <stdlib.h>
#include <math.h>
#include "fftw3.h"
#include "melodyextraction.h"

float* WindowFunction(int size)
{
	//uses Hamming Window
	float* buffer = malloc(sizeof(float) * size);
	if (buffer != NULL){ //
		for(int i = 0; i < size; ++i) {
			buffer[i] =(float) (0.54-(0.46*cos(2*M_PI*i/(size-1.0))));
		}
	}
	return buffer;
}

int NumSTFTBlocks(audioInfo info, int unpaddedWinSize, int interval)
{
	// My naive assumption is to just assume that values of zero just
	// follow the end of an audio stream (in which case, unpaddedWinSize
	// is entirely irrelevant to this calculation). Under that assumption,
	// the function should just be the following:
	//   return (int)ceil(info.frames / (float)interval);
	//
	// It's not clear to me how we arrived at the current implementation.
	// I think we arrived at it from a bunch of trial-and-error. I imagine
	// that my naive assumption may have produced some weird results & the
	// current implementation does better. If that's the case, it should be
	// documented at the very least. It's also arguable that this function
	// provide an upper bound on the number of STFT blocks and that it can
	// be reduced elsewhere (based on the pitch detection algorithm)

	int numBlocks = (int)ceil((info.frames - unpaddedWinSize)
				  / (float)interval) + 1;
	if (numBlocks < 1){
		numBlocks = 1;
	}
	return numBlocks;
}

float* Magnitude(const fftwf_complex* arr, int size)
{
	float* magArr = malloc( sizeof(float) * size);
	if (magArr != NULL){
		for(int i = 0; i < size; i++){
			magArr[i] = hypot(arr[i][0], arr[i][1]);
		}
	}
	return magArr;
}

/// helper function that determines the number of coefficients returned by a
/// real-to-complex fft on an in put containing ``bufLen`` elements
///
/// While this is a simple calculation, the reason for it can be a little
/// counter-intuitive. This is explained below:
///
/// - Consider a complex-to-complex DFT on a buffer of ``bufLen`` elements
///   This operation will calculate ``bufLen`` output coefficients.
/// - In this scenario, imagine input signal only includes real values (i.e. we
///   imaginary component is set to zero for each element).
/// - When this happens, the computed coefficient for each positive frequency
///   is entirely redundant with the coefficient for the corresponding negative
///   frequency (the exact relationship differs when ``bufLen`` is odd or even)
///
/// For these reasons, real-to-complex DFTs make an optimization and return
/// fewer coefficients.
///
/// @note
/// When ``bufLen`` is even, the output includes a coefficient for the Nyquist
/// frequency. When ``bufLen`` is odd, the output does NOT include a
/// coefficient for the Nyquist frequency.
///
/// @note
/// Note that in the case of real-to-complex DFTs, the coefficients for the
/// 0-frequency and the Nyquist Frequencies are purely real. Be aware that some
/// fft implementations get cute when handling real inputs of an even length
/// and store the real component of the Nyquist Frequency coefficient where
/// they would otherwise store the imaginary component of the 0-Frequency.
/// (this is not an issue for FFTW3).
static inline int CalcNumDFTCoef_r2c_(int bufLen) { return bufLen/2 + 1; }

int STFT_r2c(const float* input, audioInfo info, int unpaddedWinSize,
	     int paddedFFTSize, int interval, fftwf_complex** fft_data)
{
	if (unpaddedWinSize < 1) { return -1; }
	if (paddedFFTSize < unpaddedWinSize) { return -1; }

	// determine how much space is required for fft_data.
	//  - Determine how many DFTs that we will call
	const int numBlocks = NumSTFTBlocks(info, unpaddedWinSize, interval);
	//  - Determine the # of coefficients returned per DFT
	const int coefPerDFT = CalcNumDFTCoef_r2c_(paddedFFTSize);

	// For the moment, we will temporarily preserve a quirky historical
	// behavior: we will explicitly clip the Nyquist frequency from the
	// returned values (there is no need to make this adjustment when
	// paddedFFTSize is odd since it wouldn't be included anyways)
	const int nReturnedCoefsPerDFT =
		((paddedFFTSize % 2) == 0) ? coefPerDFT - 1 : coefPerDFT;

	if ((numBlocks * nReturnedCoefsPerDFT) == 0) { return -1; }

	// Now, let's actually allocate the output space
	(*fft_data) = malloc( sizeof(fftwf_complex) * numBlocks *
			      nReturnedCoefsPerDFT );
	if((*fft_data) == NULL){
		printf("malloc failed\n");
		return -1;
	}

	// set up fftw plan - note that fftwf_malloc is just a convenience
	// function that ensures that memory is allocated with appropriate
	// alignment for use of the execution path with SIMD vectorization
	float* fftw_in = fftwf_malloc( sizeof( float ) * paddedFFTSize);
	fftwf_complex* fftw_out = fftwf_malloc( sizeof( fftwf_complex ) *
						coefPerDFT );
	fftwf_plan plan  = fftwf_plan_dft_r2c_1d( paddedFFTSize, fftw_in,
						  fftw_out, FFTW_MEASURE );

	// calculate the values for the window function and allocate space to
	// store it.
	// TODO: I'm not sure that WindowFunction should use paddedFFTSize
	//       part of me really thinks that we should use unpaddedWinSize
	//       instead
	float* window = WindowFunction(paddedFFTSize);
	if (window == NULL){
		printf("windowFunc error\n");
		fflush(NULL);
		free((*fft_data));
		fftwf_destroy_plan( plan );
		fftwf_free( fftw_in );
		fftwf_free( fftw_out );
		return -1;
	}

	//run fft on each block
	for(int i = 0; i < numBlocks; i++){
		// Copy the chunk into our buffer
		int blockoffset = i*interval;
		for(int j = 0; j < paddedFFTSize; j++) {

			if (j < unpaddedWinSize && blockoffset + j < info.frames) {
				fftw_in[j] = input[blockoffset + j] * window[j];
			} else {
				//reached end of non-padded input
				//Pad the rest with 0
				fftw_in[j] = 0.0;
			}
		}

		fftwf_execute( plan );

		for (int j = 0; j < nReturnedCoefsPerDFT; j++) {
			(*fft_data)[i*nReturnedCoefsPerDFT + j][0] = fftw_out[j][0];
			(*fft_data)[i*nReturnedCoefsPerDFT + j][1] = fftw_out[j][1];
		}
	}


	free(window);
	fftwf_destroy_plan( plan );
	fftwf_free( fftw_in );
	fftwf_free( fftw_out );

	return numBlocks * nReturnedCoefsPerDFT;
}

int STFTinverse_c2r(fftwf_complex* input, audioInfo info, int winSize, int interval, float** output)
{
	//length of input is numBlocks * (winSize/2 + 1)

	fftwf_complex* fftw_in = fftwf_malloc( sizeof( fftwf_complex ) * winSize );
	float* fftw_out = fftwf_malloc( sizeof( float ) * winSize );

	fftwf_plan plan  = fftwf_plan_dft_c2r_1d( winSize, fftw_in, fftw_out, FFTW_MEASURE );

	//malloc space for output
	(*output) = calloc( info.frames, sizeof(float));
	if((*output) == NULL){
		printf("malloc failed\n");
		fftwf_destroy_plan( plan );
		fftwf_free( fftw_in );
		fftwf_free( fftw_out );
		return -1;
	}

	int numBlocks = (int)ceil((info.frames - winSize) / (float)interval) + 1;
	if(numBlocks < 1){
		numBlocks = 1;
	}

	//run fft on each block
	for(int i = 0; i < numBlocks; i++){

		// Copy the chunk into our buffer
		const int inputoffset = i*(winSize/2 + 1);
		for (int j = 0; j < winSize/2 + 1; j++) {
			fftw_in[j][0] = input[inputoffset + j][0];
			fftw_in[j][1] = input[inputoffset + j][1];
		}

		fftwf_execute( plan );

		const int outputoffset = i*interval;

		for (int j = 0; j < winSize; j++) {
			if (outputoffset + j < info.frames) {
				(*output)[outputoffset + j] += fftw_out[j] / (float)((winSize*winSize)/interval);
			}
		}
	}

	fftwf_destroy_plan( plan );
	fftwf_free( fftw_in );
	fftwf_free( fftw_out );

	return info.frames;
}
