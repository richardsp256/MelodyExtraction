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
 *
 * It's also possible that we could totally separate the roles of tripleBuffer 
 * and filterBank and have detFuncCore act as the middle man between the 2 
 * (this would reduce coupling), but it may simplify the implementation of 
 * detFuncCore to complete abstract filterBank's interactions with tripleBuffer.
 */

typedef struct filterBank filterBank;

filterBank* filterBankNew(int numChannels, int lenChannels, int overlap,
			  int samplerate, float minFreq, float maxFreq);

void filterBankDestroy(filterBank* fB);

/* we could use 2 separate functions here, but this simplifies the interface
 * and implementation */
int filterBankSetInputChunk(filterBank* fB, int length, int final_chunk);

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

/* we may need 1 more function here just in case the last chunk is basically 
 * full, and we need to shift it into an empty buffer. */


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

*/
