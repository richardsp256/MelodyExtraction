int numWindows(int winInt, int winSize, int numSamples);
int winStartSampleIndex(int winInt, int winInd);
int winStartRepSampleIndex(int winInt, int winSize, int numSamples, int winInd);
int winStopSampleIndex(int winInt, int numSamples,int winInd);
int winStopRepSampleIndex(int winInt, int winSize, int numSamples, int winInd);
int repWinIndex(int winInt, int winSize, int numSamples, int sampleIndex);
