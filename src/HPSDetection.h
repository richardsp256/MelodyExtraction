// returns 1 on success
int HarmonicProductSpectrum(float** AudioData, int size, int dftBlocksize,
			    int hpsOvr, int fftSize, int samplerate,
			    float *loudestFreq);
float BinToFreq(int bin, int fftSize, int samplerate);
