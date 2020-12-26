// returns 1 on success
// the length of loudestFreq is size / dftBlocksize
int HarmonicProductSpectrum(const float* AudioData, int size, int dftBlocksize,
			    int hpsOvr, int fftSize, int samplerate,
			    float *loudestFreq);
float BinToFreq(int bin, int fftSize, int samplerate);
