typedef float* (*PitchStrategyFunc)(double** AudioData, int size,
					     int dftBlocksize, int hpsOvr,
					     int fftSize, int samplerate);
PitchStrategyFunc choosePitchStrategy(char* name);
float* HPSDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			  int hpsOvr, int fftSize, int samplerate);
float* BaNaDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			   int hpsOvr, int fftSize, int samplerate);
float* BaNaMusicDetectionStrategy(double** AudioData, int size,
				  int dftBlocksize, int hpsOvr, int fftSize,
				  int samplerate);
