typedef float* (*FundamentalDetectionStrategy)(double** AudioData, int size,
					     int dftBlocksize, int hpsOvr,
					     int fftSize, int samplerate);
FundamentalDetectionStrategy chooseStrategy(char* name);
float* HPSDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			  int hpsOvr, int fftSize, int samplerate);
float* BaNaDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			   int hpsOvr, int fftSize, int samplerate);
float* BaNaMusicDetectionStrategy(double** AudioData, int size,
				  int dftBlocksize, int hpsOvr, int fftSize,
				  int samplerate);
float* HarmonicProductSpectrum(double** AudioData, int size, int dftBlocksize,
			       int hpsOvr, int fftSize, int samplerate);
float BinToFreq(int bin, int fftSize, int samplerate);
