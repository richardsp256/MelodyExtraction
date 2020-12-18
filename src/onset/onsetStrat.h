#include "../lists.h"

typedef int (*OnsetStrategyFunc)(float** AudioData, int size, int dftBlocksize,
			int samplerate, intList* onsets);

int TransientDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			int samplerate, intList* onsets);

