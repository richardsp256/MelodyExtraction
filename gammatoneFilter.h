void simpleGammatone(float* data, float** output, float centralFreq, int samplerate, int datalen);

void gammatoneFilter(float *x, float **bm, float cf, int fs, int nsamples);

/* the following function is identical to the preceeding function except that
 * it allows you to process the input in chunks and save the coefficients 
 * between function calls 
 */
void gammatoneFilterChunk(float *x, float **bm, float cf, int fs, int nsamples,
			  float *curr_p1r, float *curr_p2r, float *curr_p3r,
			  float *curr_p4r, float *curr_p1i, float *curr_p2i,
			  float *curr_p3i, float *curr_p4i, float *curr_qcos,
			  float *curr_qsin);
