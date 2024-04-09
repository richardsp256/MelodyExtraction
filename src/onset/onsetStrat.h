#include "../lists.h"

typedef int (*OnsetStrategyFunc)(float** AudioData, int size, int dftBlocksize,
			int samplerate, intList* onsets);

OnsetStrategyFunc chooseOnsetStrategy(char* name);

int OnsetsDSDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			int samplerate, intList* onsets);

int TransientDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			int samplerate, intList* onsets);

void AddOnsetAt(int** onsets, int* size, int value, int index );
