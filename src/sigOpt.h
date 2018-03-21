#ifndef SIGOPT_H
#define SIGOPT_H
typedef struct sigOpt sigOpt;
typedef struct sigOptChannel sigOptChannel;
typedef struct bufferIndex bufferIndex;

bufferIndex *bufferIndexCreate(int bufferNum,int index, int bufferLength);
void bufferIndexDestroy(bufferIndex *bI);
int bufferIndexGetIndex(bufferIndex *bI);
int bufferIndexGetBufferNum(bufferIndex *bI);
int bufferIndexGetBufferLength(bufferIndex *bI);
int bufferIndexIncrement(bufferIndex *bI);
int bufferIndexAdvanceBuffer(bufferIndex *bI);
bufferIndex *bufferIndexAddScalarIndex(bufferIndex *bI, int val);

int bufferIndexEq(bufferIndex *bI, bufferIndex *other);
int bufferIndexNe(bufferIndex *bI, bufferIndex *other);
int bufferIndexGt(bufferIndex *bI, bufferIndex *other);
int bufferIndexGe(bufferIndex *bI, bufferIndex *other);
int bufferIndexLt(bufferIndex *bI, bufferIndex *other);
int bufferIndexLe(bufferIndex *bI, bufferIndex *other);


sigOpt *sigOptCreate(int winSize, int hopsize, int initialOffset,
		     int numChannels, float scaleFactor);
void sigOptDestroy(sigOpt *sO);
int sigOptGetBufferLength(sigOpt *sO);
int sigOptGetSigmasPerBuffer(sigOpt *sO);
int sigOptAdvance(sigOpt *sO, float *trailingBuffer, float *centralBuffer,
                  float *leadingBuffer, int channel);
float sigOptGetSigma(sigOpt *sO, int channel);
// tells you how many time you can compute Sigma Per Buffer
int sigOptSetTerminationIndex(sigOpt *sO,int index);
int sigOptAdvanceBuffer(sigOpt *sO);

#endif /* SIGOPT_H */
