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
			       float scaleFactor, int samplerate, float minFreq,
			       float maxFreq,  char* filterStrat,
			       char* corrStrat, int dedicatedThreads);
void detFuncCoreDestroy(detFuncCore *dFC);

int detFuncCoreFirstChunkLength(detFuncCore *dFC);
int detFuncCoreNormalChunkLength(detFuncCore *dFC);
enum streamState detFuncCoreState(detFuncCore *dFC);
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

/* If we are only using 1 thread responsible for both reading in the stream and 
 * then processing it, then that thread alternates between calling 
 * detFuncCoreSetInputChunk and detFuncCoreProcessInput. In this case, the 
 * basic flow in the function is as follows:
 *    A. Iterate over all N channels. For channel i: 
 *       - call detFuncCorePSMContrib(dFC, i, 0)
 *    B. Update the detection function (May involve resizing)
 *    C. Save index pooledSummaryMatrix[pSMLength-1] for the future (it is 
 *       required for completing step B next time) and flush all entries of 
 *       pooledSummaryMatrix to 0
 *    D. Check to see if the stream has been terminated. If it has not, return 
 *       1, otherwise continue to E.
 *    E. If the last chunk had a length less than or equal to 
 *       (detFuncCoreNormalChunkLength(dFC)-overlap) skip to F. Otherwise cycle
 *       the buffers, call sigOptAdvanceBuffer, and repeat steps A->C. However 
 *       in step Aa), instead of calling filterBankProcessInput, call 
 *       filterBankPropogateFinalOverlap
 *    F. Delete the trailing buffer from filterBank. Call sigOptAdvanceBuffer.
 *    G. Complete steps A->B for the remaining entries in the array.
 * 
 * If there is at least 1 function solely dedicated to the following function, 
 * then it is not released from the following function until it has processed 
 * the entire stream. The basic control flow is very similar to the above. I 
 * will summarize it below. Note that steps labeled as SEQ indicates that only 
 * 1 thread does this operation and all other threads wait.
 *    1. SEQ - look for new input with detFuncCorePullNextChunk
 *    2. Complete step A. from above (this is parallelized)
 *    3. SEQ - Combine the pooledSummaryMatrices for all channels into 1 
 *       channel. Set the values of the pooledSummaryMatrices of all other 
 *       channels to 0.
 *    4. SEQ - Complete Steps B->C with the same thread used for step 3.
 *    5. SEQ - If the stream has not terminated, evaluate steps E->G, only 
 *       allowing the other threads to parallelize step A wherever applicable.
 * 
 * The above will definitely work with dedicated threads. After it is all 
 * implemented, judging based on how long it takes to complete step 4., it may 
 * be worthwhile to allow the other threads (assuming there is more than 1 
 * dedicated thread) to advance back to steps 1&2 if the stream is not 
 * terminated so that they are not idle. I have a feeling that we won't gain 
 * much from this.
 */

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

/* The following are just some helper functions that are only added to 
 * detFuncCore for use during unit testing. They should NEVER be called by a 
 * user.
 */

void transitionToSINGLE_CHUNK(detFuncCore *dFC, int terminationIndex);
void transitionToFIRST_CHUNK(detFuncCore *dFC, int length);
void transitionToNORMAL_CHUNK(detFuncCore *dFC, int length);
void transitionToLAST_CHUNK(detFuncCore *dFC, int terminationIndex);
