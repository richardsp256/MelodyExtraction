
int* HPSDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			  int hpsOvr, int fftSize, int samplerate);
int* BaNaDetectionStrategy(double** AudioData, int size, int dftBlocksize,
			   int hpsOvr, int fftSize, int samplerate);
int* BaNaMusicDetectionStrategy(double** AudioData, int size, int dftBlocksize,
				int hpsOvr, int fftSize, int samplerate);
int* HarmonicProductSpectrum(double** AudioData, int size, int dftBlocksize,
			     int hpsOvr);
