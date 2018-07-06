void biquadFilter(double *coef, double *x, double *y, int length);

int cascadeBiquad(int num_stages, double *coef, double *x, double *y,
		  int length);

/* The sosGammatone is another attempt at implementing an approximation 
 * for the gammatone filter.
 *
 * Although it is not currently implemented, I believe it is possible to set 
 * the coefficients of the cascaded biquad filter such that the response is 
 * normalized (the central frequency in the frequency response has fixed gain).
 *
 * In principle, the performance of this filter should be equal to or better 
 * than naive Gammatone after both have been optimized because naiveGammatone 
 * shifts the frequency of the input (creating a complex input), uses a cascade 
 * of 4 linear filters on both the real and imaginary components, and then 
 * shifts the frequency of the result. This implementation just uses 4 bilinear 
 * (second order) filters on the input. The casade of filters should be the 
 * same complexity and this implementation doesn't worry about shifting the 
 * frequency. Presently both filters require the same data doubling and data 
 * halving.
 *
 * See gammatoneFilter.c for notes on how this function can be optimized.
 *
 * Notes:
 *  - presently it recasts the input data as doubles (this is currently for 
 *    testing
 *  - presently it doubles and halves the sampling rate exactly like 
 *    naiveGammatone (it's sloppily done). I'm not sure how necessary this is. 
 *    If it is necessary and we ulimately go with this implementation then it 
 *    would be better to pass in the initial data with higher sampling rate (we 
 *    currently resample the input twice before we actually apply the 
 *    gammatone filter).
 *  - The implementation needs to be adjusted to detect and actively avoid, 
 *    overflows, underflows and NaNs (because it is recursive, if these appear, 
 *    the rest of the response will be messed up )
 *  - There may be some benefits to using a finite impulse response filter 
 *    instead of an sosGammatone
 *  - In the future (especially when/if we rewrite this to work with floats 
 *    rather
 */
void sosGammatone(float* data, float** output, float centralFreq,
		  int samplerate, int datalen);
void sosGammatoneHelper(double* data, double** output, float centralFreq,
			int samplerate, int datalen);

void sosCoef(float centralFreq, int samplerate, double *coef);

void sosGammatoneFast(float* data, float** output, float centralFreq,
		      int samplerate, int datalen);

void gammatoneIIRChunkHelper(double *coef, double *state, float* inputChunk,
			     float* filteredChunk, int nsamples);
