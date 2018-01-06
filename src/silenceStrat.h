typedef int (*SilenceStrategyFunc)(float** AudioData, int length,
				   int frameLength, int spacing,
				   int samplerate, int mode,
				   int** activityRanges);

SilenceStrategyFunc chooseSilenceStrategy(char* name);
int fVADDetectionStrategy(float** AudioData, int length, int frameLength,
			  int spacing, int samplerate, int mode,
			  int** activityRanges);
