typedef int (*OnsetStrategyFunc)(float** AudioData, int size, int dftBlocksize,
			int samplerate, int** onsets);

OnsetStrategyFunc chooseOnsetStrategy(char* name);

int OnsetsDSDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			int samplerate, int** onsets);

int TransientDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			int samplerate, int** onsets);

void AddOnsetAt(int** onsets, int* size, int value, int index );