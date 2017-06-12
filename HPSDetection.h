float* HarmonicProductSpectrum(double** AudioData, int size, int dftBlocksize,
			       int hpsOvr, int samplerate);
float BinToFreq(int bin, int dftBlocksize, int samplerate);