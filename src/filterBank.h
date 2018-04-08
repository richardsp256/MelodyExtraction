#include "gammatoneFilter.h"


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
 *
 * It's also possible that we could totally separate the roles of tripleBuffer 
 * and filterBank and have detFuncCore act as the middle man between the 2 
 * (this would reduce coupling), but it may simplify the implementation of 
 * detFuncCore to complete abstract filterBank's interactions with tripleBuffer.
 */

typedef struct filterBank filterBank;

struct filterBank* filterBankNew(int numChannels, int lenChannels, int overlap,
				 int samplerate, float minFreq, float maxFreq);

void filterBankDestroy(struct filterBank* fB);

float* centralFreqMapper(int numChannels, float minFreq, float maxFreq);

/* The following 3 functions are used for processing inputChunks. The first 
 * function is used when you feed in the very first chunk. The second function
 * is used when you feed in all other chunks except the last chunk. The third 
 * function is used when you feed in the last chunk 
 *
 * for all functions, nsamples is the number of samples in the inputChunk
 */

void filterBankFirstChunk(filterBank* fB, float* inputChunk,
			  int nsamples, float** leadingSpectraChunk);

void filterBankUpdateChunk(filterBank* fB, float* inputChunk,
			   int nsamples, float** leadingSpectraChunk,
			   float** trailingSpectraChunk);

void filterBankFinalChunk(filterBank* fB, float* inputChunk,
			  int nsamples, float** leadingSpectraChunk,
			  float** trailingSpectraChunk);


