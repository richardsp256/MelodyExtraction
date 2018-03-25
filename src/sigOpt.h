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

/* Call the following function once for each channel at the beginning. */
int sigOptSetup(sigOpt *sO, int channel, float *buffer);

/* When you call sigOptAdvanceWindow, the buffer in which you are calculating 
 * the correntropy (the buffer where the sigOpt window is centered) is always 
 * passed as centralBuffer. The function expects values of NULL if the user 
 * does not presently have access to a trailingBuffer or a leadingBuffer.
 */
float sigOptAdvanceWindow(sigOpt *sO, float *trailingBuffer,
			  float *centralBuffer, float *leadingBuffer,
			  int channel);

float sigOptGetSigma(sigOpt *sO, int channel);
/* tells you how many time you compute Sigma Per Buffer */
int sigOptSetTerminationIndex(sigOpt *sO,int index);

int sigOptAdvanceBuffer(sigOpt *sO);


/* The following function is designed for debugging. It computes all of the 
 * values at once. 
 */
int sigOptFullRollSigma(int initialOffset, int hopsize, float scaleFactor,
			int winSize, int dataLength, int numWindows,
			float *input, float **sigma);

#endif /* SIGOPT_H */
