typedef float* (*OnsetStrategyFunc)(double** AudioData, int size,
					     int dftBlocksize, int hpsOvr,
					     int fftSize, int samplerate);
OnsetStrategyFunc chooseOnsetStrategy(char* name);
float* OnsetsDSDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			  int hpsOvr, int fftSize, int samplerate);