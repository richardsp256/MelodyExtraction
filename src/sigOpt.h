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

/* This constructs sigOpt.
 * Basic Requirements: 
 *     winSize > 3
 *     winSize - floor(winSize/2) - 1 > initialOffset >= 0
 *     winSize - floor(winSize/2) - 1 >= hopsize > 0
 *     winSize % hopsize = 0
 *     scaleFactor > 0
 *
 *     The entire length of the processed stream must be at least the length of 
 *     hopsize.
 *
 * Potential Implementation Improvements:
 *     - Allow the user to specify their own bufferLength. The bufferLength we 
 *       calculate internally is the minimum allowed bufferLength for the 
 *       implementation. We would just require that bufferLength be at least as 
 *       large as that value and that bufferLength % hopsize = 0.
 *     - Remove the requirement that winSize % hopsize = 0. This is a relic of 
 *       a very old implementation of sigOpt (one that was never committed) and 
 *       I don't think it adds anything).
 *     - Accuracy related Change: Currently, we advance one edge of the window 
 *       all the way across hopsize at a time before advancing the other window
 *       edge. We should probably alternate advancement of windows edges.
 *     - Speed related Change: rather than incrementing instances of 
 *       bufferIndex 1 value at a time while advancing the window, we could 
 *       update the bufferIndex values at the end of advancement
 *     - Speed related Change: This would be a large overhaul (probably not 
 *       worth the speed gain). Because of the requirements we set up the 
 *       advancement of the windows follow a very precise pattern. We could 
 *       refactor sigOpt to use finite state machines to control how each 
 *       window edge should be advanced for each channel. In doing this, we 
 *       could entirely remove the bufferIndex objects from our code. See the 
 *       top of sigOpt.c for further discussion on how this can be achieved.
 */
sigOpt *sigOptCreate(int winSize, int hopsize, int initialOffset,
		     int numChannels, float scaleFactor);


void sigOptDestroy(sigOpt *sO);


int sigOptGetBufferLength(sigOpt *sO);


/* tells you how many time you compute Sigma Per Buffer */
int sigOptGetSigmasPerBuffer(sigOpt *sO);


/* Indicates where the last index of the stream is found. A returned value of 1 
 * indicates success. Negative values indicate errors.
 * Notes: 
 *     - The termination index CANNOT be equal to the bufferLength. In the case 
 *       where you would want to set it equal to bufferLength, you would wait 
 *       until the last filled buffer is the central buffer and then you would 
 *       call this function and set the last buffer to 0.
 *     - If the entire stream is shorter than bufferLength, then this function 
 *       must be called before sigOptSetup is called.
 *     - This function should NOT be called on an instance of sigOpt while 
 *       functions are being called in other threads that manipulate any of the
 *       properties of sigOpt (namely sigOptSetup, sigOptAdvanceWindow, 
 *       and sigOptAdvanceBuffer)
 */
int sigOptSetTerminationIndex(sigOpt *sO,int index);


/* Call the following function once for each channel at the beginning. 
 * After calling this, you MUST Advance the window once.
 * A returned value of 1 indicates success. Negative values indicate errors.
 */
int sigOptSetup(sigOpt *sO, int channel, float *buffer);

/* When you call sigOptAdvanceWindow, the buffer in which you are calculating 
 * the correntropy is always passed as centralBuffer. The function expects 
 * values of NULL if the user does not presently have access to a 
 * trailingBuffer or a leadingBuffer. The function returns the value of sigma 
 * after the window has been advanced. If a value<=0 is returned, then 
 * an error occured and the current usage of sigOpt, should be cleaned up.
 */
float sigOptAdvanceWindow(sigOpt *sO, float *trailingBuffer,
			  float *centralBuffer, float *leadingBuffer,
			  int channel);

/* Computes the value of Sigma for the Window where the sigOpt is currently 
 * located. 
 */
float sigOptGetSigma(sigOpt *sO, int channel);

/* call this when the leading buffer becomes the central buffer and the 
 * central buffer becomes the trailing buffer.
 * Note:
 *     - This function should NOT be called on an instance of sigOpt while 
 *       functions are being called in other threads that manipulate any of the
 *       properties of sigOpt (namely sigOptSetup, sigOptAdvanceWindow, 
 *       and sigOptAdvanceBuffer)
 */
int sigOptAdvanceBuffer(sigOpt *sO);


/* The following function is designed for debugging. It computes all of the 
 * values at once. 
 */
int sigOptFullRollSigma(int initialOffset, int hopsize, float scaleFactor,
			int winSize, int dataLength, int numWindows,
			float *input, float **sigma);

#endif /* SIGOPT_H */
