#include "gammatoneFilter.h"
#include "tripleBuffer.h"

float* centralFreqMapper(int numChannels, float minFreq, float maxFreq);

/* Here is the plan for writing this, I will reimplement the filterBank so it 
 * works similarly to how it once did (I don't actually think that will require 
 * much effort). Then I want to adapt the filterBank to work with the 
 * tripleBuffer.
 * 
 * How filterBank fits into the overall design.
 * --------------------------------------------
 * The ultimate goal is to create a class-like struct called detFuncCore. This 
 * struct will keep track of filterBank, tripleBuffer, and sigOpt, in addition 
 * to an array holding the resulting detection Function (or most recent chunk 
 * of a detection function (it's not completely clear yet).
 *
 * Anyway, the plan is that every time detFuncCore recieves a new chunk of the 
 * audio stream, it passes the chunk of audio to filterBank which then stores 
 * it. Either at this time, or shortly before hand, detFuncCore calls 
 * tripleBufferCycle.
 * Next, one (or more) thread(s) iterates through all 64 channels and performs 
 * some kind of operation. In this operation, it tells filterBank to compute 
 * the filtered input for the given channel, and it supplies the filterBank 
 * with the tripleBuffer (at which time filterBank updates the tripleBuffer). 
 * Then, the trailing, central, and leading buffers are loaded
 * from tripleBuffer, and they are used to compute the correntropy everywhere 
 * within the central buffer.
 */

enum streamState {
	NO_CHUNK,     // before any chunks have been set in the filterBank
	FIRST_CHUNK,  // The very first chunk has been added - there is no
	              // overlap between buffers
	NORMAL_CHUNK, // The current chunk is neither the first nor the last
	LAST_CHUNK,   // The current chunk is the final chunk - the end of the
	              // chunk must be zero-padded
	SINGLE_CHUNK, // The current chunk is both the first and the last CHUNK
};

typedef struct filterBank filterBank;

filterBank* filterBankNew(int numChannels, int lenChannels, int overlap,
			  int samplerate, float minFreq, float maxFreq);

void filterBankDestroy(filterBank* fB);

float filterBankCentralFreq(filterBank* fB, int channel);

/* we could use 2 separate functions here, but this simplifies the interface
 * and implementation */
int filterBankSetInputChunk(filterBank* fB, float* input, int length,
			    int final_chunk);

/* we add the following 2 functions to our interface in case we choose to 
 * implement the filter bank using FIR filters. In that case, we would require 
 * slightly more overlap between buffers than we already have. In the case that
 * we use IIR filters, they return obvious results.
 *
 * These following 2 functions return the required size of the input chunks.
 */
int filterBankFirstChunkLength(filterBank *fB);
int filterBankNormalChunkLength(filterBank *fB);

/* The following function is the function always called for processing 
 * inputChunks. The filterBank internally accounts for if the input chunk is 
 * the first, normal, or final input chunk.
 */
int filterBankProcessInput(filterBank *fB, tripleBuffer *tB, int channel); 

/* This last function here is provided just in case the last chunk is nearly 
 * full. This function would be used to propogate all values in the last 
 * (filterBankFirstChunkLength(fB) - filterBankNormalChunkLength(fB)) elements
 * into the next buffer. 
 */

int filterBankPropogateFinalOverlap(filterBank *fB, tripleBuffer *tB,
				    int channel);

/* The following function is only included for debugging purpose. The 
 * filterBank is used to process an entire input stream all at once. 
 * The output is a floating point array with length numChannels*dataLen.
 *
 * The filtered result for the ith channel begins at index i*dataLen of the 
 * output.
 */

float *fullFiltering(int numChannels, int lenChannels, int overlap,
		     int samplerate, float minFreq, float maxFreq,
		     float* input, int dataLen);
