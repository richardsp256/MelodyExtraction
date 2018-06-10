#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "detFuncCore.h"

// We have placed a requirement that the entire stream is at least twice the
// length of the correntropy window if the stream is shorter than the first
// buffer.

/* The following 2 macros affect the memory allocation and reallocation for the 
 * detFunc member of detFuncCore.
 * Specifically, we initially allocate memory for 
 * (1 + INITIAL_DFL_EXTRA_CHUNKS) chunks of audio. Consequently, 
 * INITIAL_DFL_EXTRA_CHUNKS must be zero or greater.
 * Later, during reallocation, we always grow the array to hold
 * (1 + GROWTH_DFL_EXTRA_CHUNKS) additional chunks. Consequently, 
 * GROWTH_DFL_EXTRA_CHUNKS must also be zero or greater.
 *
 * We should probably use some kind of growth factor instead of a constant 
 * number of chunks for updating the size of the detection function, with the 
 * guaruntee that there is alway enough space for an integer number of chunks.
 */
#define INITIAL_DFL_EXTRA_CHUNKS 1
#define GROWTH_DFL_EXTRA_CHUNKS 1

// the following give the default values of the lastPSMEntry and
// terminationIndex members of detFuncCore. The values are unattainable through
// normal means and they indicate that the detection function of the first
// audio chunk has not yet been calculated, and that the stream has not been
// terminated, respectively
#define DEFAULT_LASTPSMENTRY -1.f
#define DEFAULT_TERMINATIONINDEX -1

typedef int (*dFCHelperFuncPtr)(detFuncCore *dFC);
/* The following functions are tracked by detFuncCore based on its state. */
int detFuncCoreProcessNoChunk(detFuncCore *dFC);
int detFuncCoreProcessFirstChunk(detFuncCore *dFC);
int detFuncCoreProcessNormalChunk(detFuncCore *dFC);
int detFuncCoreProcessLastChunk(detFuncCore *dFC);
int detFuncCoreProcessSingleChunk(detFuncCore *dFC);

int detFuncCoreResizeDetFuncDefault(detFuncCore *dFC);
int detFuncCoreResizeDetFuncNormal(detFuncCore *dFC);
int detFuncCoreResizeDetFuncTerminal(detFuncCore *dFC);

typedef int (*dFCSetInputChunkFunc)(detFuncCore *dFC,  float* input, int length,
				    int final_chunk);
typedef int (*pSMContribStrat)(detFuncCore *dFC, int channel, int thread,
			       int numPSMWin, int centralBuffIndex);
int calcPSMContrib(detFuncCore *dFC, int channel, int thread, int numPSMWin,
		   int centralBuffIndex);

struct detFuncCore{
	enum streamState state;
	dFCHelperFuncPtr processChunk_Func;
	dFCHelperFuncPtr resizeDetFunc_Func;

	int numChannels;
	int hopsize;
	int corrWinSize;
	float *pooledSummaryMatrix;
	int pSMLength;
	pSMContribStrat pSMContribFunc;

	// the following 2 variables are defined for the purposes of
	// identifying where the stream terminates. We could come up with a way
	// to determine this without tracking these quantities.
	long streamLength;
	int terminationIndex;

	float lastPSMEntry;
	int detFuncSize;
	int detFuncFilledLength;
	float *detFunc;

	int dedicatedThreads;

	tripleBuffer *tB;
	filterBank *fB;
	sigOpt *sO;
};

/* A consequence of using subobjects is that the majority of the information 
 * necessary for computing values are stored in the subobjects.
 * It is not always clear to me whether we should be duplicating this 
 * information in the detFuncCore struct, or accessing it from the subobjects. 
 * (For intensive operations the former might be better, but otherwise the 
 * latter is typically better). 
 *
 * I want to be able to modify these decisions in the future so I have factored 
 * them out into the following static inline functions.
 */

// Could get the following from sigOpt
static inline int detFuncCoreHopsize(detFuncCore *dFC){
	return dFC->hopsize;
}

// Could get this from sigOpt - its equal to the number of sigmas
static inline int detFuncCorePSMLength(detFuncCore *dFC){
	return dFC->pSMLength;
}

// Could get the following from any subObject
static inline int detFuncCoreNumChannels(detFuncCore *dFC){
	return dFC->numChannels;
}


/* Some utility functions that will likely be repeated frequently */

static inline float* detFuncCoreGetPSM(detFuncCore *dFC, int thread_num){
	return ((dFC->pooledSummaryMatrix) +
		(thread_num * detFuncCorePSMLength(dFC)));
}



/* Onto implementing the interface */

detFuncCore *detFuncCoreCreate(int correntropyWinSize, int hopsize,
			       int numChannels, int sigWinSize,
			       float scaleFactor, int samplerate, float minFreq,
			       float maxFreq, char* filterStrat,
			       char* corrStrat, int dedicatedThreads){

	if (dedicatedThreads < 0){
		return NULL;
	} else if (dedicatedThreads !=0){
		// temporary
		return NULL;
	}

	if (correntropyWinSize <= 0){
		return NULL;
	}
	if (hopsize <= 0){
		return NULL;
	}
	if (numChannels <=0){
		return NULL;
	}

	char *temp_name = strdup(corrStrat);
	if (!temp_name){
		printf("Error: Not enough memory to duplicate corrStrat\n");
		return NULL;
	}
	for(int i = 0; temp_name[i]; i++){
		temp_name[i] = tolower(temp_name[i]);
	}
	pSMContribStrat pSMContribFunc;
	if (strcmp(temp_name,"default")==0) {
		pSMContribFunc = &calcPSMContrib;
		free(temp_name);
	} else {
		free(temp_name);
		printf("Error: The corrStrat argument can only be equal to "
		       "\"default\"\n");
		return NULL;
	}

	/* No need to check sigWinSize, scaleFactor, samplerate, minFreq, or
	 * maxFreq. None of these values are directly employed by detFuncCore; 
	 * they are all used in the construction of the subobjects, which will 
	 * flag any problems.
	 */

	// Begin the Creation
	detFuncCore *dFC = malloc(sizeof(detFuncCore));

	if (dFC == NULL){
		return NULL;
	}

	// we should probably think about this more, but for now we set the
	// center of the region for which we compute sigma to be the following
	int initialOffset = correntropyWinSize/2;

	// first contruct sigOpt
	dFC->sO = sigOptCreate(sigWinSize, hopsize, initialOffset,
			       numChannels, scaleFactor);

	if (dFC->sO == NULL){
		free(dFC);
		return NULL;
	}

	// construct the pooledSummaryMatrix
	int pSMLength = sigOptGetSigmasPerBuffer(dFC->sO);
	if (dedicatedThreads == 0){
		dFC->pooledSummaryMatrix = calloc(pSMLength, sizeof(float));
	} else {
		dFC->pooledSummaryMatrix = calloc(pSMLength * dedicatedThreads,
						  sizeof(float));
	}

	free(dFC->pooledSummaryMatrix);

	if (dFC->pooledSummaryMatrix == NULL){
		sigOptDestroy(dFC->sO);
		free(dFC);
		return NULL;
	}

	/* Now calculate the bufferLength and construct the tripleBuffer
	 * The addition of 1 is intentional - correntropy is calculated using
	 * the 2* correntropyWinSize elements AFTER the starting index. */
	int bufferLength = pSMLength * 2 * correntropyWinSize + 1;
	dFC->tB = tripleBufferCreate(numChannels, bufferLength);
	if (dFC->tB == NULL){
		free(dFC->pooledSummaryMatrix);
		sigOptDestroy(dFC->sO);
		free(dFC);
		return NULL;
	}

	int overlap = bufferLength - sigOptGetBufferLength(dFC->sO);
	// Construct the filterBank
	dFC->fB = filterBankNew(numChannels, bufferLength, overlap,
				samplerate, minFreq, maxFreq, filterStrat);

	if (dFC->fB == NULL){
		free(dFC->pooledSummaryMatrix);
		sigOptDestroy(dFC->sO);
		tripleBufferDestroy(dFC->tB);
		free(dFC);
		return NULL;
	}

	dFC->numChannels = numChannels;
	dFC->hopsize = hopsize;
	dFC->corrWinSize = correntropyWinSize;
	dFC->pSMLength = pSMLength;
	dFC->pSMContribFunc = pSMContribFunc;
	dFC->dedicatedThreads = dedicatedThreads;

	dFC->streamLength = 0;
	dFC->terminationIndex = DEFAULT_TERMINATIONINDEX;
	dFC->lastPSMEntry = DEFAULT_LASTPSMENTRY;

	dFC->state = NO_CHUNK;
	dFC->processChunk_Func = &detFuncCoreProcessNoChunk;
	dFC->resizeDetFunc_Func = &detFuncCoreResizeDetFuncDefault;

	/* here we will allocate the initial memory for detFunc.
	 * Note that detection function will always have 1 fewer entry than 
	 * the total number of pooledSummaryMatrix windows that have been 
	 * calculated
	 */
	dFC->detFuncFilledLength = 0;
	dFC->detFuncSize = pSMLength * (INITIAL_DFL_EXTRA_CHUNKS + 1) - 1;
	dFC->detFunc = malloc(sizeof(float) * dFC->detFuncSize);
	if (dFC->detFunc == NULL){
		detFuncCoreDestroy(dFC);
		return NULL;
	}
	return dFC;
}

void detFuncCoreDestroy(detFuncCore *dFC){
	if (dFC->detFunc != NULL){
		// we will set detFunc to NULL after returning it
		free(dFC->detFunc);
	}

	free(dFC->pooledSummaryMatrix);
	tripleBufferDestroy(dFC->tB);
	sigOptDestroy(dFC->sO);
	filterBankDestroy(dFC->fB);
	free(dFC);
}

int detFuncCoreFirstChunkLength(detFuncCore *dFC){
	return filterBankFirstChunkLength(dFC->fB);
}

int detFuncCoreNormalChunkLength(detFuncCore *dFC){
	return filterBankNormalChunkLength(dFC->fB);
}


/* I think we are going to add another state aspect to filterBank. It's going 
 * to be related to the value of dedicatedThreads.
 * This involves 2 functions: 
 *    - detFuncCoreSetInputChunk: an interface function
 *    - detFuncCorePullNextChunk: an internal helper function.
 * 
 * If there are no dedicated threads, then all input chunks are added to 
 * detFuncCore via detFuncCoreSetInputChunk. detFuncCorePullNextChunk does 
 * nothing and always succeeds
 *
 * If there is at least 1 dedicated thread, then the detFuncCoreSetInputChunk 
 * should never be called. Instead, the chunk will be provided through some 
 * external streamBuffer. It is accessed by calling detFuncCorePullNextChunk.
 */


/* Below is a helper function that:
 * - control state transitions
 * - advances the tripleBuffer (either adds new buffer or cycles it)
 * - calls sigOptAdvanceBuffer(dFC->sO) (if starting state is NORMAL_CHUNK)
 * - adds the new input chunk in filterBank
 */
int detFuncCorePrepareNextChunk(detFuncCore *dFC, float *input, int length,
				int final_chunk){
	if (length < 0){
		return -2;
	} else if ((final_chunk != 0) && final_chunk != 1) {
		return -3;
	}

	switch(dFC->state){
	case LAST_CHUNK :
		return -4;
	case SINGLE_CHUNK :
		return -4;
	case NO_CHUNK :
		/* Can Become FIRST_CHUNK or SINGLE_CHUNK */

		/* add buffer to tripleBuffer */
		tripleBufferAddLeadingBuffer(dFC->tB);

		if (final_chunk == 0){
			if (length != detFuncCoreFirstChunkLength(dFC)){
				return -5;
			}
			dFC->state = FIRST_CHUNK;
			dFC->processChunk_Func = &detFuncCoreProcessFirstChunk;
			dFC->resizeDetFunc_Func =
				&detFuncCoreResizeDetFuncDefault;
		} else {

			if (length > detFuncCoreFirstChunkLength(dFC)){
				return -6;
			} else if (length < (dFC->corrWinSize*2)){
				// could probably do better
				printf("The entire stream must be at least "
				       "twice the length of the correntropy "
				       "window.\n");
				return -7;
			}

			dFC->terminationIndex = length;
			// all further dealings with the termination Index are
			// handled later
			dFC->state = SINGLE_CHUNK;
			dFC->processChunk_Func = &detFuncCoreProcessSingleChunk;
			dFC->resizeDetFunc_Func =
				&detFuncCoreResizeDetFuncTerminal;
		}
		break;
	default :
		/* Only accesed by FIRST_CHUNK or NORMAL CHUNK
		 * Both cases only allow transitions to NORMAL_CHUNK or 
		 * LAST_CHUNK */

		if (tripleBufferNumBuffers(dFC->tB) == 3){
			tripleBufferCycle(dFC->tB);
		} else {
			tripleBufferAddLeadingBuffer(dFC->tB);
		}

		// check that the input length is valid

		if (final_chunk == 0){
			if (length != detFuncCoreNormalChunkLength(dFC)){
				return -5;
			}
			if (dFC->state == NORMAL_CHUNK){
				sigOptAdvanceBuffer(dFC->sO);
			}

			dFC->state = NORMAL_CHUNK;
			dFC->processChunk_Func = &detFuncCoreProcessNormalChunk;
			dFC->resizeDetFunc_Func =
				&detFuncCoreResizeDetFuncNormal;
		} else {
			if (length > detFuncCoreNormalChunkLength(dFC)){
				return -6;
			}
			if (dFC->state == NORMAL_CHUNK){
				sigOptAdvanceBuffer(dFC->sO);
			}
			dFC->terminationIndex = length;
			// all further dealings with the termination Index are
			// handled later
			dFC->state = LAST_CHUNK;
			dFC->processChunk_Func = &detFuncCoreProcessLastChunk;
			dFC->resizeDetFunc_Func =
				&detFuncCoreResizeDetFuncTerminal;
		}

	}
	// update internal steamLength
	dFC->streamLength = dFC->streamLength + length;

	printf("Unclear if I should update numWindowsProcessed Now or later\n");
	return -1;

	// add input chunk to filterBank
	filterBankSetInputChunk(dFC->fB, input, length, final_chunk);

	return 1;
}


int detFuncCoreSetInputChunk(detFuncCore* dFC, float* input, int length,
			     int final_chunk){

	
	
	if (dFC->dedicatedThreads == 0){
		return detFuncCorePrepareNextChunk(dFC, input, length,
						   final_chunk);
	} else {
		/* This always automatically fails. This is not the way to pass
		 * the input chunks to detFuncCore if it has dedicated threads. 
		 */
		printf("The function, detFuncCoreSetInputChunk, should not be "
		       "called if dFC has at least 1 dedicated thread.\n");
		return 0;
	}
}

/* This is a helper function. It is used for retrieving the next Chunk if the 
 * detFuncCore has at least 1 dedicated thread. If it doesn't then, this 
 * function just returns True; the same role is carried out by the interface 
 * function detFuncCoreSetInputChunk 
*/
int detFuncCorePullNextChunk(detFuncCore* dFC){

	if (dFC->dedicatedThreads == 0){
		return 1;
	} else {
		/* Pulls the next chunk from the external Stream Buffer, if 
		 * applicable.
		 * If the next chunk is not yet ready (unlikely), hang until it
		 * is ready.
		 * Pass the next chunk to detFuncCorePrepareNextChunk
		 *
		 * Needs to be implemented
		 */
		printf("Have not implemented the helper function, "
		       "detFuncCorePullNextChunk, \nfor use when detFuncCore "
		       "has at least 1 dedicated thread.\n"); 
		return -1;
	}
}

/* The bottom is a helper function. For some thread, it computes the 
 * contribution of channel to the PSM for an entire input chunk.
 * If the chunk for which the PSM is being calculated is the final input chunk,
 * then needs to take slightly special care to only compute PSM Contrib to the 
 * end of the chunk (and not any farther)
 *
 * In this function: 
 * first get the trailingBuffer, centralBuffer, and leadingBuffer
 * for every element we wish to fill in PSMcontrib:
 *     sigma= sigOptAdvanceWindow(sO, trailingBuffer,
 *                                centralBuffer, leadingBuffer,
 *                                channel);
 *     set element in PSMcontrib equal to itself + PSM contribution from the 
 *         channel being processed
 * 
 * Probably need to indicate which buffer in the triple Buffer is the central 
 * buffer. For nearly every case:
 *    - if there are 2 buffers, then its the first buffer
 *    - if there are 3 buffers, then its the second buffer
 * the only exception is during the processing of the very last buffer.
 */

#define M_1_SQRT2PI 0.3989422804f
float calcPSMEntryContrib(float* x, int winSize, float sigma)
{
	/* this is ripped from fastGaussTransform.h. We need to implement this 
	 * function in a separate file so that it can be called for this file, 
	 * for simpleDetFunc and for fastGaussTransform
	 */
	int i, j;
	float out, temp;
	float denom = -0.5/(sigma * sigma);
	out = 0;
	for (i = 0; i < winSize; i++){
		for (j = 1; j <= winSize; j++){
			temp = x[i] - x[i+j];
			out += expf(temp * temp * denom);
		}
	}
	out *= M_1_SQRT2PI / sigma;
	return out;
}

int calcPSMContrib(detFuncCore *dFC, int channel, int thread, int numPSMWin,
		   int centralBuffIndex){

	// load the trailing, central, and leading buffers
	float *trailingBuffer, *centralBuffer, *leadingBuffer;
	centralBuffer = tripleBufferGetBufferPtr(dFC->tB, centralBuffIndex,
						 channel);
	if (centralBuffIndex == 0){
		trailingBuffer = NULL;
		if (tripleBufferNumBuffers(dFC->tB) == 1){
			leadingBuffer=NULL;
		} else if (tripleBufferNumBuffers(dFC->tB) == 2){
			leadingBuffer = tripleBufferGetBufferPtr(dFC->tB, 1,
								 channel);
		} else {
			// sanity check
			printf("Unexpected number of buffers.\n");
			return -1;
		}		
	} else if (centralBuffIndex == 1){
		trailingBuffer = tripleBufferGetBufferPtr(dFC->tB, 0, channel);
		if (tripleBufferNumBuffers(dFC->tB) == 2){
			leadingBuffer=NULL;
		} else if (tripleBufferNumBuffers(dFC->tB) == 3){
			leadingBuffer= tripleBufferGetBufferPtr(dFC->tB, 2,
								channel);;
		} else {
			//sanity check
			printf("tripleBuffer must have 2 or 3 buffers.\n");
		}

	} else {
		// sanity check
		printf("Unexpected centralBuffIndex\n");
		return -1;
	}
	int corrWinSize = dFC->corrWinSize;
	int hopsize = detFuncCoreHopsize(dFC);
	float *pSM = detFuncCoreGetPSM(dFC, thread);

	int start = 0;
	for (int i = 0; i<numPSMWin; i++){
		float sigma = sigOptAdvanceWindow(dFC->sO, trailingBuffer,
						  centralBuffer, leadingBuffer,
						  channel);
		pSM[i] += calcPSMEntryContrib(centralBuffer+start,
					      corrWinSize, sigma);
		start += hopsize;
	}
	return 1;
}

int identifyCentralBuffIndex(detFuncCore *dFC){
	/* this is the default behavior we want to use to identify the central 
	 * buffer index. (The very last chunk will be handled separately).
	 * We want the following mapping from NumBuffers:
	 *    numBuffers = 1 -> return 0
	 *    numBuffers = 2 -> return 0
	 *    numBuffers = 3 -> return 1 
	 */
	return (tripleBufferNumBuffers(dFC->tB) - 1)/2;
}

/* The following is a helper function that either sets up sigOpt or computes 
 * correntropy. The choice between these 2 functionalities is determined by the
 * value passed to numPSMWin. If the value is 0, then sigOpt is set up. 
 * Otherwise, calcPSMContrib is called to compute correntropy contributions in 
 * numPSMWin Correntropy calculations.
 * The argument fBfunc controls if and how the tripleBuffer is modified for 
 * every channel. If we are processing the very last input chunk, and the 
 * tripleBuffer has already been modified with the last input, then fBfunc 
 * should be set to NULL. If we are treating a normal chunk or the very first 
 * chunk, then fBfunc should be set to filterBankProcessInput. If the final 
 * input chunk was at least as long as detFuncCoreNormalChunkLength, then we 
 * can also set fBfunc to filterBankPropogateFinalOverlap.
 *
 * If we adopt a simple master-slave thread pool parallelization model, then 
 * the following function must simply be modified.
 */

typedef int (*fBProcessPtr)(filterBank *fB, tripleBuffer *tB, int channel);

// centralBuffIndex indicates the index of the central buffer in dFC->tB
int detFuncCoreProcessInputHelper(detFuncCore *dFC, fBProcessPtr fBfunc,
				  int numPSMWin,int centralBuffIndex){
	if (dFC->dedicatedThreads != 0){
		printf("Have not implemented the helper function, "
		       "detFuncCoreProcessInputHelper, \nfor use when "
		       "detFuncCore has at least 1 dedicated thread.\n"); 
		return -1;
	}

	for (int channel = 0; channel<detFuncCoreNumChannels(dFC); channel++){
		int temp;
		if (fBfunc != NULL){
			temp = fBfunc(dFC->fB, dFC->tB, channel);
			if (temp!=1){
				printf("There has been a problem with "
				       "execution of fBfunc\n");
				return temp;
			}
		}

		if (numPSMWin>0){
			temp = (dFC->pSMContribFunc)(dFC, channel, 0, numPSMWin,
						     centralBuffIndex);
			//temp = calcPSMContrib(dFC, channel, 0, numPSMWin,
			//		      centralBuffIndex);
			if (temp!=1){
				printf("There has been a problem with "
				       "dFC->pSMContribFunc\n");
				return temp;
			}
		} else {
			temp = sigOptSetup(dFC->sO, channel,
					   tripleBufferGetBufferPtr(dFC->tB, 0,
								    channel));
			if (temp!=1){
				printf("There has been a problem with "
				       "sigOptSetup\n");
				return temp;
			}
		}
	}
	return 1;
}

int detFuncCoreProcess(detFuncCore* dFC, int thread_num){
	if (dFC->dedicatedThreads == 0){
		return dFC->processChunk_Func(dFC);
	} else {
		/* If we decide to use a master slave thread pool, the idea is 
		 * that we would assign one of the threads the master status.
		 */
		printf("Have not implemented the helper function, "
		       "detFuncCorePullNextChunk, \nfor use when detFuncCore "
		       "has at least 1 dedicated thread.\n");
		return -1;
	}
}

/* The following function is a helper function in which all information about 
 * the pooledSummaryMatrix is consolidated. This means that:
 * - In the threaded case, the entries of PSMbuffers are all added together in 
 *   the entries of the PSMbuffer of a single thread.
 */

int detFuncCoreConsolidatePSMEntries(detFuncCore *dFC){
	if (dFC->dedicatedThreads == 0){
		return 1;
	} else {
		printf("Have not implemented the helper function, "
		       "detFuncCoreConsolidatePSMInfo, \nfor use when "
		       "detFuncCore has at least 1 dedicated thread. \n");
		/* Need to add the pooledSummaryMatrices together so that one 
		 * thread has a pooledSummaryMatrix with all accumulated 
		 * entries. This can be done entirely sequentially or partially
		 * parralellized. 
		 */
		return -1;
	}
}

int totalNumPSMEntriesHelper(detFuncCore *dFC){
	/* Helper function that the computes the total number of 
	 * pooledSummaryMatrix values to be computed.
	 * This should only be called once terminationIndex has been set. 
	 */
	if (dFC->terminationIndex == DEFAULT_TERMINATIONINDEX){
		return -1;
	}
	return (int)ceil(((double)(dFC->streamLength
				      - (long)dFC->corrWinSize))
			    / (double)detFuncCoreHopsize(dFC)) + 1;
}

int numberPSMEntriesFinalChunkHelper(detFuncCore *dFC){
	/* Helper function that computes the total number of remaining
	 * pooledSummaryMatrix elements that should be computed for the 
	 * very last chunk.
	 * This should only be called once termination index has been set 
	 *
	 * The implementation could probably be modified so that we don't need 
	 * need to know the entire length of the stream*/

	int totalLength = totalNumPSMEntriesHelper(dFC);
	if (totalLength == -1){
		return -1;
	}

	/* first we will compute the total number of PSM entries to be computed
	 */
	int numWindows = totalNumPSMEntriesHelper(dFC);
	int pSMLength = detFuncCorePSMLength(dFC);

	if (numWindows % pSMLength != 0){
		return numWindows % pSMLength;
	} else {
		return pSMLength;
	}
}

// Define helper functions for resizing the detection function
// We could consolidate these into a single function that updates based on the
// internal state of detFuncCore

int resizeHelper(detFuncCore *dFC, int newLength){
	float* temp = realloc(dFC->detFunc, sizeof(float) * newLength);
	if (temp == NULL){
		free(dFC->detFunc);
		return -1;
	} else { 
		dFC->detFuncFilledLength = newLength;
		dFC->detFunc = temp;
		return 1;
	}
}

// this is a dummy function used during NO_CHUNK and FIRST_CHUNK
int detFuncCoreResizeDetFuncDefault(detFuncCore *dFC){
	return 0;
}

int detFuncCoreResizeDetFuncNormal(detFuncCore *dFC){

	if ((dFC->detFuncFilledLength == 0) ||
	    ((dFC->detFuncSize - dFC->detFuncFilledLength) >=
	     detFuncCorePSMLength(dFC))){
		// resizing is not necessary
		return 1;
	}
	int newLength = (dFC->detFuncFilledLength +
			 (GROWTH_DFL_EXTRA_CHUNKS + 1) *
			 detFuncCorePSMLength(dFC));
	return resizeHelper(dFC, newLength);
}

// this is a helper function to be during LAST_CHUNK or SINGLE_CHUNK when the
// termination index is known
int detFuncCoreResizeDetFuncTerminal(detFuncCore *dFC){
	int newLength = totalNumPSMEntriesHelper(dFC) - 1;

	if (newLength == dFC->detFuncSize){
		// resizing is not necessary (this is unlikely to occur)
		return 1;
	}

	if (newLength <1){
		printf("Error - attempting to resize detFunc down to a "
		       "size of zero\n");
		return -2;
	} else if (newLength < dFC->detFuncSize){
		printf("Error - attempting to resize detFunc to a length "
		       "smaller than its current length\n");
		return -3;
	}
	return resizeHelper(dFC, newLength);
}

int updateDetFunc(detFuncCore *dFC, int activeResize){
	// Handles the updating of detection function
	// stores the final entry in pooledSummaryMatrix for later

	// active resize is an int, of value 0 or 1
	// it should always be 1, except for the very last chunks, where we are
	// attempting to resize the detection function just once
	
	if (activeResize == 1){
		int temp = (dFC->resizeDetFunc_Func)(dFC);
		if (temp!=1){
			printf("Problem during detection function resize\n");
			return temp;
		}
	}

	int extraLength = detFuncCorePSMLength(dFC);
	if (dFC->lastPSMEntry == DEFAULT_LASTPSMENTRY){
		// this means that we are processing the very first chunk
		// at most the length of the detFunc will increase by
		// detFuncCorePSMLength(dFC)-1
		extraLength--;
	}

	if (dFC->terminationIndex != DEFAULT_TERMINATIONINDEX){
		// this means that the length could be shorter since the stream
		// is terminated
		int temp = (dFC->detFuncSize-dFC->detFuncFilledLength);
		if (temp < extraLength){
			extraLength = temp;
		}
	}

	int start = dFC->detFuncFilledLength;
	float* pSM = dFC->pooledSummaryMatrix;
	int nIter = extraLength;
	
	// now time to fill in the detection function
	if ((dFC->lastPSMEntry != DEFAULT_LASTPSMENTRY) && (nIter>0)){
		dFC->detFunc[start] = pSM[0] -  dFC->lastPSMEntry;
		start++;
		nIter--;
	}

	float* curDetFunc = dFC->detFunc + start;

	for (int i=0;i<nIter; i++){
		curDetFunc[i] = pSM[i+1]-pSM[i];
	}

	// finally, let's update the filled length and lastPSMEntry
	dFC->detFuncFilledLength += extraLength;
	// if this was the very last chunk then it is irrelevant what we set
	// lastPSMEntry equal to
	dFC->lastPSMEntry = pSM[detFuncCorePSMLength(dFC) -1];

	return 1;
		
}

int flushPSMEntries(detFuncCore *dFC){
	/* Handles the process of setting all entries in pooledSummaryMatrix
	 * to 0 */
	if (dFC->dedicatedThreads == 0){
		float *pooledSummaryMatrix = detFuncCoreGetPSM(dFC, 0);
		for (int i=0; i<detFuncCorePSMLength(dFC); i++){
			pooledSummaryMatrix[i] = 0.f;
		}
		return 1;
	} else {
		printf("Have not implemented the helper function, "
		       "flushPSMEntries, \nfor use when "
		       "detFuncCore has at least 1 dedicated thread. \n");
		return -1;
	}
}

int detFuncCoreProcessPenultimateSectionHelper(detFuncCore *dFC,
					       fBProcessPtr fBfunc,
					       int activeResize){
	/* This is a helper function called by detFuncCoreProcessLastChunk or
	 * detFuncCoreProcessSingleChunk
	 *
	 * The third argument indicates whether or not the detection function 
	 * gets resized during this function call
	 */

	/* Need to implement where we get the terminationIndex from */
	int terminationIndex = dFC->terminationIndex;

	/* Set the terminationIndex in the tripleBuffer and sigOpt */
	int temp = tripleBufferSetTerminalIndex(dFC->tB, terminationIndex);
	if (temp != 1){
		printf("Error while calling tripleBufferSetTerminalIndex\n");
		return temp;
	}

	temp = sigOptSetTerminationIndex(dFC->sO,terminationIndex);
	if (temp != 1){
		printf("Error while calling sigOptSetTerminationIndex\n");
		return temp;
	}

	/* Process the section */
	int centralBuffIndex = identifyCentralBuffIndex(dFC);
	temp = detFuncCoreProcessInputHelper(dFC, fBfunc, dFC->pSMLength,
					     centralBuffIndex);
	if (temp != 1){
		return temp;
	}

	/* Update detection function */
	temp = updateDetFunc(dFC, activeResize);
	if (temp != 1){
		return temp;
	}

	/* Flush PSM Entries. */
	temp = flushPSMEntries(dFC);
	if (temp != 1){
		return temp;
	}
	return 1;
}

int detFuncCoreProcessFinalSectionHelper(detFuncCore *dFC, int activeResize){
	/* This is a helper function called by detFuncCoreProcessLastChunk or
	 * detFuncCoreProcessSingleChunk 
	 *
	 * The second argument indicates whether or not the detection function 
	 * gets resized during this function call
	 */

	if (tripleBufferNumBuffers(dFC->tB) == 3){
		tripleBufferRemoveTrailingBuffer(dFC->tB);
	}

	/* Process the section - the final section has already been calculated
	 * If there are any problems, it likely has to do with how we set 
	 * centralBuffIndex, or with assuming the final chunk has already been 
	 * processed in SINGLE_CHUNK mode */
	int finalPSMWindows = numberPSMEntriesFinalChunkHelper(dFC);
	int centralBuffIndex = tripleBufferNumBuffers(dFC->tB) - 1;
	int temp = detFuncCoreProcessInputHelper(dFC, NULL, finalPSMWindows,
					     centralBuffIndex);
	if (temp != 1){
		return temp;
	}

	/* Update detection function */
	temp = updateDetFunc(dFC, activeResize);
	if (temp != 1){
		return temp;
	}

	return 1;
}

int detFuncCoreProcessNoChunk(detFuncCore *dFC){
	return 0;
}

int detFuncCoreProcessFirstChunk(detFuncCore *dFC){
	/* The helper function will just setup sigOpt and apply the filter to 
	 * the very first input chunk */
	return detFuncCoreProcessInputHelper(dFC, filterBankProcessInput, 0,
					     0);
}

int detFuncCoreProcessNormalChunk(detFuncCore *dFC){
	/* compute the PooledSummary Matrix entries */

	int centralBuffIndex = identifyCentralBuffIndex(dFC);
	int temp = detFuncCoreProcessInputHelper(dFC, filterBankProcessInput,
						 dFC->pSMLength,
						 centralBuffIndex);
	if (temp != 1){
		return temp;
	}
	/* The following is only important if the detection Function Core is 
	 * parallelized using a master-slave thread pool:
	 * temp = detFuncCoreConsolidatePSMEntries(dFC);
	 * if (temp != 1){
	 *	return temp;
	 * }
	 */

	/* Update detection function, we always want to check if the detection 
	 * function must be resized */
	temp = updateDetFunc(dFC,1);
	if (temp != 1){
		return temp;
	}

	/* Flush PSM Entries. */
	temp = flushPSMEntries(dFC);
	if (temp != 1){
		return temp;
	}
	return 1;
}

int detFuncCoreProcessLastChunk(detFuncCore *dFC){

	int temp;
	fBProcessPtr fBfunc;
	int activeResize;
	if (dFC->terminationIndex >= detFuncCoreNormalChunkLength(dFC)){
		temp = detFuncCoreProcessNormalChunk(dFC);
		if (temp != 1){
			return temp;
		}

		if (tripleBufferNumBuffers(dFC->tB) == 3){
			tripleBufferCycle(dFC->tB);
		} else {
			tripleBufferAddLeadingBuffer(dFC->tB);
		}

		temp = sigOptAdvanceBuffer(dFC->sO);
		if (temp != 1){
			printf("An error occured during the first "
			       "sigOptAdvanceBuffer\n");
			return temp;
		}

		fBfunc = filterBankPropogateFinalOverlap;
		dFC->terminationIndex -= detFuncCoreNormalChunkLength(dFC);

		/* During the detFuncCoreProcessNormalChunk function call, the 
		 * detection function was already resized for the final time. 
		 * We do not want to attempt to resize it again during the 
		 * subsequent call of 
		 * detFuncCoreProcessPenultimateSectionHelper. Doing so would 
		 * not cause any errors, but it would waste time.
		 */
		activeResize = 0;
	} else {
		fBfunc = filterBankProcessInput;

		/* The detection function must be resized during the call of 
		 * detFuncCoreProcessPenultimateSectionHelper 
		 */
		activeResize = 1;
	}

	temp = detFuncCoreProcessPenultimateSectionHelper(dFC, fBfunc,
							  activeResize);
	if (temp != 1){
		return temp;
	}

	temp = sigOptAdvanceBuffer(dFC->sO);
	if (temp != 1){
		printf("An error occured during sigOptAdvanceBuffer\n");
		return temp;
	}

	/* at this point we never want to resize the detection function. It 
	 * should always been taken care of by now */
	return detFuncCoreProcessFinalSectionHelper(dFC,0);
}

int detFuncCoreProcessSingleChunk(detFuncCore *dFC){

	int temp;
	int terminationIndex = dFC->terminationIndex;
	int activeResize;

	if (terminationIndex >= detFuncCoreNormalChunkLength(dFC)){
		/* The entire stream is longer than a normal ChunkLength (but 
		 * smaller than the expected length of the first chunk). This 
		 * means we need to filter the input, transfer as much as 
		 * possible into the first tripleBuffer buffer, preprocess 
		 * sigOpt and then transfer the remainder into the second 
		 * tripleBuffer. 
		 *
		 * If we won't do this then sigOpt will not account for the 
		 * stream that occurs after the normal ChunkLength in its 
		 * calculations and it's possible that correntropy calculations
		 * involving windows starting after the normal Chunk Length 
		 * will be missed. */
		
		/* need to filter the stream and handle sigOpt preprocessing */
		temp = detFuncCoreProcessInputHelper(dFC,
						     &filterBankProcessInput, 0,
						     0);
		if (temp != 1){
			return temp;
		}

		// Now, update the terminationIndex
		dFC->terminationIndex -= detFuncCoreNormalChunkLength(dFC);
		terminationIndex = dFC->terminationIndex;

		/* Now, let's propogate the remaining stream after normal Chunk
		 * Length and compute correntropy */
		fBProcessPtr fBfunc = filterBankPropogateFinalOverlap;

		/* Here we process the penultimate section (coincidentally, 
		 * it's the very first section). During this processing, the 
		 * detection function will be resized to its final length 
		 */
		temp = detFuncCoreProcessPenultimateSectionHelper(dFC,
								  fBfunc, 1);
		if (temp != 1){
			return temp;
		}

		temp = sigOptAdvanceBuffer(dFC->sO);
		if (temp != 1){
			printf("An error occured during sigOptAdvanceBuffer\n");
			return temp;
		}
		/* The detection function does not need to be resized again. */
		activeResize = 0;
	} else {
		/* Set the terminationIndex in the tripleBuffer and sigOpt */
		int temp = tripleBufferSetTerminalIndex(dFC->tB,
							terminationIndex);
		if (temp != 1){
			printf("Error while calling "
			       "tripleBufferSetTerminalIndex\n");
			return temp;
		}

		temp = sigOptSetTerminationIndex(dFC->sO,terminationIndex);
		if (temp != 1){
			printf("Error while calling "
			       "sigOptSetTerminationIndex\n");
			return temp;
		}

		/* need to filter the stream and handle sigOpt preprocessing */
		temp = detFuncCoreProcessInputHelper(dFC,
						     &filterBankProcessInput, 0,
						     0);
		if (temp != 1){
			return temp;
		}
		/* The detection function must be resized to its final size 
		 * while processing the first and only section. */
		activeResize = 1;
	}

	/* Now lets process the input in the last buffer */
	return detFuncCoreProcessFinalSectionHelper(dFC,activeResize);
}


float *detFuncCoreGetDetectionFunction(detFuncCore* dFC, int *length){
	if (dFC->dedicatedThreads != 0){
		/* This function would need to wait until all of the threads 
		 * are complete */
		printf("Have not implemented the interface function, "
		       "detFuncCoreGetDetectionFunction, \nfor use when "
		       "detFuncCore has at least 1 dedicated thread.\n");
		return NULL;
	}

	if (dFC->detFuncSize != dFC->detFuncFilledLength){
		// sanity check
		printf("Something went wrong. detFuncSize must be equal to "
		       "detFuncFilledLength.\n");
	}
	*length = dFC->detFuncFilledLength;

	float* out = dFC->detFunc;
	dFC->detFunc = NULL;
	return out;
}
