float* HarmonicProductSpectrum(double** AudioData, int size, int dftBlocksize,
			       int hpsOvr, int fftSize, int samplerate);
float BinToFreq(int bin, int fftSize, int samplerate);