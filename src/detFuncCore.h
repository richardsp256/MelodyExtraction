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
 * track of the interface functions to the subobjects to allow for mocking and 
 * real unit testing, but its probably more effort than its worth. In that case,
 * we probably would have needed another set of files for the full 
 * implementation.
 */

typedef struct detFuncCore detFuncCore;

detFuncCore *detFuncCoreCreate();
void detFuncCoreDestroy(detFuncCore * dFC);

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
