typedef void (*OnsetStrategyFunc)(float** AudioData, int size, int dftBlocksize,
			int spacing, int samplerate);

OnsetStrategyFunc chooseOnsetStrategy(char* name);

void OnsetsDSDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			int spacing, int samplerate);