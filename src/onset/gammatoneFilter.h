void biquadFilter(double *coef, const double *x, double *y, int length);

void cascadeBiquad(int num_stages, double *coef, const double *x, double *y,
		   int length);

/// The sosGammatone is an IIR approximation for the gammatone filter.
///
/// The sos Gammatone Filter is described by Slaney (1993):
///    ( https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf )
/// Our entire implementation is based on his description (It is implemented 
/// using a cascade of 4 biquad filters or using 4 second order sections - sos).
///
/// @param[in] input the input data to be filtered
/// @param[out] output an array to be filled with the output data
/// @param[in] centralFreq The frequency of the gammatone Filter
/// @param[in] samplerate The sampling rate of the input
/// @param[in] datalen The number of entries in input
/// @returns 0 upon success
///
/// Although it is not currently implemented, I believe it is possible to set 
/// the coefficients of the cascaded biquad filter such that the response is 
/// normalized (the central frequency in the frequency response has fixed gain).
///
/// Implementation Notes:
/// - presently it recasts the input data as doubles for improved accuracy. We
///   need to be a little careful about precision and stability, but we could
///   probably switch to using just doubles.
/// - This implementation also doubles and halves the sampling rate. Depending
///   on the input, we could actually run into aliasing problems due to the
///   Nyquist Frequency being close to the highest central frequency. For now,
///   we just upsample and downsample in a kind of sloppy way (not exactly
///   right, but  close). If this turns out to be important, than we would
///   probably just be better off simply using input with a higher samplerate.
/// - The implementation should be adjusted to detect and actively avoid, 
///   overflows, underflows and NaNs (due to the recursive nature, the
///   appearance will spread throughout the output
///
/// @par Optimization Notes:
/// This function can certainly be optimized. Currently the cascadeBiquad
/// calls biquadFilter 4 separate times (meaning that we iterate over the array
/// 4 times). By refactoring, we only need to make one pass through the array. 
/// We could also optimize biquadFilter to always expect a0 = 1 (which is
/// standard) which would remove `4*datalen` multiplications. If we tailored
/// the implementation to this function in particular, we could also take
/// advantage of the fact that b2 is always 0.
///
/// @par Alternative Implementations
/// There may be some benefits to using a FIR filter instead.  
///
/// @par
/// Additionally, it is definitely worth considering an alternative IIR
/// approximatoin. The All-Pole Gammatone Filter (APGF) is also described by
/// Slaney (1993):
///   ( https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf )
/// and it should achieve better complexity. I believe additional discussion of
/// the APGF can be found in Lyon (1996):
///   ( http://www.dicklyon.com/tech/Hearing/APGF_Lyon_1996.pdf
/// Lyon (1996) also discuses how the One-Zero Gammatone Filter (OZGF) which is
/// slightly modified from the APGF and is supposed to be a slightly better
/// approximation for a gammatone.
int sosGammatone(const float* data, float* output, float centralFreq,
		 int samplerate, int datalen);
void sosGammatoneHelper(const double* data, double* output, float centralFreq,
			int samplerate, int datalen);

void sosCoef(float centralFreq, int samplerate, double *coef);


/// A faster implementation of sosGammatone that performs all operations with
/// floats and does not resample. See the documentation for sosGammatone for
/// additional information.
int sosGammatoneFast(const float* data, float* output, float centralFreq,
		     int samplerate, int datalen);
