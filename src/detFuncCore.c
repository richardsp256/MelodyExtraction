#include "detFuncCore.h"


typedef int (*dFCProcessChunkFunc)(detFuncCore *dFC, int channel);
/* The following functions are tracked by detFuncCore based on its state. */
int detFuncCoreProcessNoChunk(detFuncCore *dFC, int channel);
//int detFuncCoreProcessFirstChunk(detFuncCore *dFC, int channel);
//int detFuncCoreProcessNormalChunk(detFuncCore *dFC, int channel);
//int detFuncCoreProcessLastChunk(detFuncCore *dFC, int channel);
//int detFuncCoreProcessSingleChunk(detFuncCore *dFC, int channel);

struct detFuncCore{
	enum streamState state;
	dFCProcessChunkFunc processChunkFunc;

	int numChannels;
	int hopsize;
	int corrWinSize;
	float *pooledSummaryMatrix;
	int pSMLength;

	int detFuncSize;
	int detFuncFilledLength;
	float *detFunc;

	int numThreads; // this probably should not be here right now
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

static inline int detFuncCoreIsTerminated(detFuncCore *dFC){
	return tripleBufferIsTerminatedStream(dFC->tB);
}

static inline int detFuncCoreGetTerminalIndex(detFuncCore *dFC){
	return tripleBufferGetTerminalIndex(dFC->tB);
}

/* Some utility functions that will likely be repeated frequently */

static inline float* detFuncCoreGetPSM(detFuncCore *dFC, int thread_num){
	return ((dFC->pooledSummaryMatrix) +
		(thread_num * detFuncCorePSMLength(detFuncCore *dFC)));
}



/* Onto implementing the interface */

detFuncCore *detFuncCoreCreate(int correntropyWinSize, int hopsize,
			       int numChannels, int sigWinSize,
			       float scaleFactor, int samplerate,
			       float minFreq, float maxFreq,
			       int dedicatedThreads){
	if (dedicatedThreads !=0){
		// temporary
		return NULL;
	}

	// CHECK THAT ALL OF ARGUMENTS that this function explicitly uses (the
	// subobjects can handle arguments that are simply passed through)


	// Begin the Creation
	detFuncCore *dFC = malloc(sizeof(detFuncCore)):

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
		dFC->pooledSummaryMatrix = malloc(sizeof(pSMLength));
	} else {
		dFC->pooledSummaryMatrix = malloc(sizeof(pSMLength)
						  * dedicatedThreads);
	}

	// should probably using calloc - otherwise set all values to 0
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
				samplerate, minFreq, maxFreq);

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
	dFC->numThreads = numThreads;

	dFC->state = NO_CHUNK;
	dFC->processChunkFunc = &detFuncCorePreprocess;

	// we should actually set a real value for detFunc
	// this needs to be changed
	dFC->detFunc =NULL;
	dFC->detFuncSize = 0;
	dFC->detFuncFilledLength = 0;
	return dFC;
}

void *detFuncCoreDestroy(detFuncCore *dFC){
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

/* If we are only using 1 thread responsible for both reading in the stream and 
 * then processing it, then that thread alternates between calling 
 * detFuncCoreSetInputChunk and the following function. In this case, the basic 
 * flow in the function is as follows:
 *    A. Iterate over all N channels. For channel i: 
 *       a) call filterBankProcessInput(dFC->fB,dFC->tB,i)
 *       b) load the trailing_buffer, central_buffer, and leading_buffers for 
 *          channel i from tripleBuffer. (Depending on the total number of 
 *          buffers, up to 2 buffers may be set to NULL
 *       c) iterate over j=0 up to j<pSMLength. For index j, compute the 
 *          correntropy starting at central_buffer[j] and add it to 
 *          pooledSummaryMatrix[j]
 *    B. Update the detection function (May involve resizing)
 *    C. Save index pooledSummaryMatrix[pSMLength-1] for the future (it is 
 *       required for completing step B next time) and flush all entries of 
 *       pooledSummaryMatrix to 0
 *    D. Check to see if the stream has been terminated. If it has not, return 
 *       1, otherwise continue to E.
 *    E. If the last chunk had a length less than or equal to 
 *       (detFuncCoreNormalChunkLength(dFC)-overlap) skip to F. Otherwise cycle
 *       the buffers and repeat steps A->C. However in step Aa), instead of 
 *       calling filterBankProcessInput, call filterBankPropogateFinalOverlap
 *    F. Delete the trailing buffer from filterBank.
 *    G. Complete steps A->B for the remaining entries in the array.
 * 
 * If there is at least 1 function solely dedicated to the following function, 
 * then it is not released from the following function until it has processed 
 * the entire stream. The basic control flow is very similar to the above. I 
 * will summarize it below. Note that steps labeled as SEQ indicates that only 
 * 1 thread does this operation and all other threads wait.
 *    1. SEQ - look for new input
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
int detFuncCoreProcess(detFuncCore* dFC, int thread_num){
	if (dFC->dedicatedThreads == 0){
		return 1;
	} else {
		return 0;
	}
}

/* The following function is explicitly called by detFuncCoreProcessFirstChunk
 * and detFuncCoreProcessSingleChunk
 */
int detFuncCorePreprocess(detFuncCore *dFC, int channel){
	return 1;
}

int detFuncCoreProcessNoChunk(detFuncCore *dFC, int channel){
	return 0;
}
