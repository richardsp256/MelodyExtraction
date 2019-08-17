
int ResampledLength(int len, float sampleRatio);
int Resample(float* input, int len, float sampleRatio, float *output);
int ResampleAndAlloc(float** input, int len, float sampleRatio,
		     float **output);
