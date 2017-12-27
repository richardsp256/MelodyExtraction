typedef float* (*PitchStrategyFunc)(float** AudioData, int size,
				    int dftBlocksize, int hpsOvr,
				    int fftSize, int samplerate);
PitchStrategyFunc choosePitchStrategy(char* name);
float* HPSDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			    int hpsOvr, int fftSize, int samplerate);
float* BaNaDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			     int hpsOvr, int fftSize, int samplerate);
float* BaNaMusicDetectionStrategy(float** AudioData, int size,
				  int dftBlocksize, int hpsOvr, int fftSize,
				  int samplerate);
