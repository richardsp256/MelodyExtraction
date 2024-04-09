
int ResampledLength(int len, float sampleRatio);
int Resample(const float * input, int len, float sampleRatio, float *output);
int ResampleAndAlloc(const float * input, int len, float sampleRatio,
		     float **output);
