#include "tripleBuffer.h"
#include "filterBank.h"
#include "sigOpt.h"

/* detFuncCore is the struct and interface for computing the correntropy 
 * detection function of a data stream.
 * 
 * The interface is subject to change somewhat, going forward, but it conveys 
 * the basic idea of what is going on.
 * We have ultimately decided to have detFuncCore handle the resizing of the 
 * resulting array because it would have been more of a hassel to have an 
 * external thread do that (unless the length of the output had a max length).
 *
 * We will need to address threading once we achieve basic functionallity, but 
 * the interface has been planned so that minimal changes will be required. 
 *
 * We have toyed around with having a struct with the sole purpose of keeping 
 * track of the interface functions to the subObjects to allow for mocking and 
 * true unit testing, but it may be more effort than its worth. 

 * I think it would only have been worth doing if we had chosen a unit testing 
 framework that supports mocking, like CMocka. If we were to do this, we would 
 * need to declare the following function pointers:
 *
 * 
 * tripleBuffer - 10 functions
 * typedef tripleBuffer* (*tBNewPtr)(int num_channels, int buffer_length);
 * typedef void (*tBDestroyPtr)(tripleBuffer *tB);
 * typedef int (*tBGeneralPtr)(tripleBuffer *tB);
 * - for NumBuffers, AddLeadingBuffer, RemoveTrailingBuffer, Cycle, 
 *   IsTerminatedStream, and GetTerminalIndex
 * typedef float(*tBGetBufferPtr)(tripleBuffer *tB, int bufferIndex,
 *                                int channelNum);
 * typedef int (*tBSetTerm)(tripleBuffer *tB, int terminalBufferIndex);

 * sigOpt - 8 functions
 * typedef sigOpt* (*sONewPtr)(int winSize, int hopsize, int initialOffset,
 *                             int numChannels, float scaleFactor);
 * typedef void (*sODestroyPtr)(sigOpt *sO);
 * typedef int (*sOGeneralPtr)(sigOpt *sO);
 * - for bufferLength, GetSigmasPerBuffer, and Advance Buffer
 * typedef int (*sOSetTermPtr)(sigOpt *sO,int index);
 * typedef int (*sOSetupPtr)(sigOpt *sO, int channel, float *buffer);
 * typedef int (*sOAdvWinPtr)(sigOpt *sO, float *trailingBuffer,
 *                            float *centralBuffer, float *leadingBuffer,
 *                            int channel);
 *
 * filterBank - 7 functions
 * typedef filterBank* (*fBNewPtr)(int numChannels, int lenChannels, 
 *                                 int overlap, int samplerate, float minFreq,
 *                                 float maxFreq);
 * typedef void (*fBDestroyPtr)(filterBank* fB);
 * typedef int (*fBSetInputChunkPtr)(filterBank* fB, float* input, int length,
 *                                   int final_chunk);
 * typedef int (*fBChunkLengthPtr)(filterBank *fB);
 * - for Normal and First chunks
 * typedef int (*fBProcessPtr)(filterBank *fB, tripleBuffer *tB, int channel);
 * - for both processInput and propogateFinal Overlap
 */

typedef struct detFuncCore detFuncCore;


/* 2 Main use case: 
 * 1. Only having one thread responsible for both reading in the stream and 
 *    then processing it. dedicatedThreads = 0.
 * 2. Having 1 thread assigned to reading in the stream and having N threads 
 *    assigned to detFuncCore for processing it. dedicatedThreads = N.
 */
detFuncCore *detFuncCoreCreate(int correntropyWinSize, int hopsize,
			       int numChannels, int sigWinSize,
			       float scaleFactor, int samplerate,
			       float minFreq, float maxFreq,
			       int dedicatedThreads);

int detFuncCoreFirstChunkLength(detFuncCore *dFC);
int detFuncCoreNormalChunkLength(detFuncCore *dFC);

/* Once we start implementing the threading, we may want to revisit how we add 
 * new input chunks.
 * I tend to think that a better way to do things would be to have some kind of 
 * queue or intermediary buffer to add the audio to, that way detFuncCore can 
 * pull the next chunk as soon as it needs it. 
 *
 * There are 2 other obvious benefits to doing this:
 *    1. We could modify this inputBuffer to read in a file automatically
 *    2. We could take care of any resampling in this inputBuffer
 */
int detFuncCoreSetInputChunk(detFuncCore* dFC, float* input, int length,
			     int final_chunk);

/* This function would be designed for actually processing the input. If 
 * running this without threading, we would alternate calling this function and 
 * setting the input function. If we use threading, the idea would be that the 
 * threads would call this function and then not be released until the entire 
 * stream is processed. 
 *
 * Internally this will also take care of the setup after the very first chunk 
 * has been provided.
 */
int detFuncCoreProcessInput(detFuncCore* dFC, int thread_num);

/* This function actually retrieves the full detection function at the end.
 */
float *detFuncCoreGetDetectionFunction(detFuncCore* dFC, int *length);
