typedef float* (*PitchDetectionStrategyFunc)(double** AudioData, int size,
					     int dftBlocksize, int hpsOvr,
					     int fftSize, int samplerate);
PitchDetectionStrategyFunc chooseStrategy(char* name);
float* HPSDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			  int hpsOvr, int fftSize, int samplerate);
float* BaNaDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			   int hpsOvr, int fftSize, int samplerate);
float* BaNaMusicDetectionStrategy(double** AudioData, int size,
				  int dftBlocksize, int hpsOvr, int fftSize,
				  int samplerate);
