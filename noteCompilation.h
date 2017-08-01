int calcNoteRanges(int* onsets, int onset_size, int* activityRanges,
		   int aR_size, int** noteRanges, int samplerate);
int assignNotePitches(float* freq, int length, int* noteRanges, int nR_size,
		      int winInt, int winSize, int numSamples,
		      float** noteFreq);
float averageFreq(int startSample, int stopSample, int winInt, int winSize,
		  int numSamples, float *freq, int length);
float medianFreq(int startSample, int stopSample, int winInt, int winSize,
		 int numSamples, float *freq, int length);
int* noteRangesEventTiming(int* noteRanges, int nR_size, int sample_rate,
			   int bpm, int division);
