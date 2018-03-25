#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sigOpt.h"

/* For simplicity, windowSize must be an integral multiple of hopsize. If this 
 * changes, then the fact the right edge of the window will always be on the 
 * buffer when calculating the sigma for the first interval of the buffer.
 *
 * A buffer will always include (windowSize/hopsize//2 + 1)*hopsize+leftOffset
 * indices
 *
 * I have decided that for simplicity, we will be using the bufferIndex object 
 * I am about to create. By doing this, the implementation of sigma optimizer 
 * will be considerably simplified but will consist of a lot of checking if 
 * indices fall inside the window. The movement of window follows a standard 
 * cadence and we could definitely get some more speed if we implemented the 
 * sigma optimizer as a state machine. However, that would involve 
 * significantly more work and I am not sure if the speed up would be worth the 
 * effort.
 *
 * Basically the sigmaOptimizer can be thought of as a state machine with 6 
 * states based on advancing the window by the hopsize. 
 *
 * For the most part preparation with the initial buffer doesn't change, 
 * however special care must be taken for cases 5 and 6.
 *
 * Advancement Cases:
 * 1. Normal Case - 3 buffers. The center of the window is always contained on 
 *                  the center buffer. The left window edge uses the indices on 
 *                  the trailing buffer and at the very end comes to be on the  
 *                  central buffer. The right window edge for the very first 
 *                  interval may be on the central buffer and then for all 
 *                  remaining intervals is always on the leading buffer.
 * 2. Initialization Case - 2 buffers. This happens most of the time whenever 
 *                          we initialize operations. The center and left edge 
 *                          of the window are entirely contained on the 
 *                          trailing buffer.
 * 3. Almost Ending Case - 3 buffers. The stream terminates somewhere on the 
 *                         leading buffer.
 * 4. Ending Case - 2 buffers. This happens most of the time whenever we are 
 *                  finishing up operations. The left window edge is on the 
 *                  trailing buffer (and possibly the leading buffer) while 
 *                  the center and right window edge are always on the leading 
 *                  buffer. The stream terminates on the leading buffer.
 * 5. 2-Buffer Stream - Like initialization case (2 buffers), but the entire 
 *                      stream only fits into 2 buffers. This means that the 
 *                      termination index lies in the second buffer.
 * 6. 1-Buffer Stream - The entire stream fits into 1 buffer.
 *
 * The idea behind the operation is that the sigmaOptimizer can inform the user 
 * how many advancements it can make before it needs further input (advancing 
 * to a new buffer)
 *
 * Advancing the window is broken down into 2 parts - advancing the left edge 
 * and advancing the right edge. For the first window, samples are only ever 
 * added
 * 
 * During setup (when we only have 1 buffer), we add all of the elements of the 
 * first buffer. Special Care must be taken in case we are handling a 1-Buffer 
 * Stream (Advancement case 6).
 *
 * During the very first advancement (case 2), the en
 */


struct bufferIndex{
	int buffer_num;
	int index;
	int buffer_length; // we don't really need to track this, but its more
	                   // convenient to track then constantly passing it in
	                   // as an argument.
};

bufferIndex *bufferIndexCreate(int bufferNum,int index, int bufferLength){
	if (index <0) {
		return NULL;
	} else if (index >= bufferLength){
		return NULL;
	} else if (bufferLength <= 0) {
		return NULL;
	}
	bufferIndex *bI = malloc(sizeof(bufferIndex));
	bI->buffer_num = bufferNum;
	bI->index = index;
	bI->buffer_length = bufferLength;
	return bI;
}

void bufferIndexDestroy(bufferIndex *bI){
	free(bI);
}

int bufferIndexGetIndex(bufferIndex *bI){
	return bI->index;
}

int bufferIndexGetBufferNum(bufferIndex *bI){
	return bI->buffer_num;
}

int bufferIndexGetBufferLength(bufferIndex *bI){
	return bI->buffer_length;
}

int bufferIndexIncrement(bufferIndex *bI){
	//if (((bI->index +1) == bI->bufferLength) &&
	//    (bI->buffer_num == INT_MAX)){
	//     return 0;
	// }
	(bI->index)++;
	if (bI->index == bI->buffer_length){
		bI->index = 0;
		(bI->buffer_num)++;
	}
	return 1;
}

int bufferIndexAdvanceBuffer(bufferIndex *bI){
	(bI->buffer_num)--;
	return 1;
}

bufferIndex *bufferIndexAddScalarIndex(bufferIndex *bI, int val){
	if (val<0){
		return NULL;
	}

	int buffer_length = bI->buffer_length;
	int index = bI->index+val;
	int buffer_num = bI->buffer_num + (index / buffer_length);
	index = index % buffer_length;
	return bufferIndexCreate(buffer_num,index, buffer_length);
}

int bufferIndexModifyVal(bufferIndex *bI, int buffer_num, int index){
	/* This function modifies the values of an existing bufferIndex object.
	 */
	if (index <0) {
		return 0;
	} else if (index >= bI->buffer_length){
		return 0;
	}

	bI->buffer_num = buffer_num;
	bI->index = index;
	return 1;
}

int bufferIndexEq(bufferIndex *bI, bufferIndex *other){
	if (((bI->buffer_num) == (other->buffer_num)) &&
	    ((bI->index) == (other->index)) &&
	    ((bI->buffer_length) == (other->buffer_length))){
		return 1;
	}
	return 0;
}

int bufferIndexNe(bufferIndex *bI, bufferIndex *other){
	if (((bI->buffer_num) == (other->buffer_num)) &&
	    ((bI->index) == (other->index)) &&
	    ((bI->buffer_length) == (other->buffer_length))){
		return 0;
	}
	return 1;
}

int bufferIndexGt(bufferIndex *bI, bufferIndex *other){
	if ((bI->buffer_length) != (other->buffer_length)){
		return 0;
	}

	if (((bI->buffer_num) == (other->buffer_num)) &&
	    ((bI->index) > (other->index))){
		return 1;
	} else if ((bI->buffer_num) > (other->buffer_num)) {
		return 1;
	}
	return 0;
}

int bufferIndexGe(bufferIndex *bI, bufferIndex *other){
	if ((bI->buffer_length) != (other->buffer_length)){
		return 0;
	}

	if (((bI->buffer_num) == (other->buffer_num)) &&
	    ((bI->index) >= (other->index))){
		return 1;
	} else if ((bI->buffer_num) > (other->buffer_num)) {
		return 1;
	}
	return 0;
}

int bufferIndexLt(bufferIndex *bI, bufferIndex *other){
	if ((bI->buffer_length) != (other->buffer_length)){
		return 0;
	}

	if (((bI->buffer_num) == (other->buffer_num)) &&
	    ((bI->index) < (other->index))){
		return 1;
	} else if ((bI->buffer_num) < (other->buffer_num)) {
		return 1;
	}
	return 0;
}

int bufferIndexLe(bufferIndex *bI, bufferIndex *other){
	if ((bI->buffer_length) != (other->buffer_length)){
		return 0;
	}

	if (((bI->buffer_num) == (other->buffer_num)) &&
	    ((bI->index) <= (other->index))){
		return 1;
	} else if ((bI->buffer_num) < (other->buffer_num)) {
		return 1;
	}
	return 0;
}



struct sigOpt{
	int winSize;
	int sizeLeft;
	int sizeRight;
	int hopsize;
	float scaleFactor;
	int initialOffset; // The number of indices from the left of the start
	                   // of the first calculaton interval that is
	                   // considered the center of the window which is the
	                   // center of the window.
	int bufferLength;
	int bufferTerminationIndex;
	int numChannels;
	sigOptChannel *channels;
};

struct sigOptChannel{
	int nobs;
	double mean_x;
	double ssqdm_x;
	bufferIndex *windowLeft;
	bufferIndex *windowRight;
	int leftEdgeCounter; // this is used to determine when we must start advancing the left edge of the window.
};

int numZeroLeftEdgeAdvancements(int hopsize, int sizeLeft, int initialOffset){
	/* This tells you how many times we advance the window calculate sigma,
	 * before the left edge has a non-zero value. This accounts for the 
	 * fact that we advance the window before EVERY sigma calculation -
	 * including the first calculation 
	 */
	return (sizeLeft - initialOffset)/hopsize + 1;
}

int computeSigOptBufferLength(int initialOffset, int winSize, int hopsize){
	/* initialOffset: is the number of indices of the offset from the left 
	 *                edge of the very start of the stream where we 
	 *                consider the center of the first window to be. 
	 * winSize:       is the window size used in sigOpt in units of 
	 *                samples. It must be an integer multiple of hopsize. 
	 *                If it is not, then -1 is returned. Furthermore, 
	 *                winSize must be greater than the sum of initialOffset 
	 *                and hopsize (technically it only NEEDS to be greater 
	 *                than hopsize, but by requiring it to be greater than 
         *                initialOffset + hopsize, it simplifies some behavior).
	 *                If it is not, then -2 is returned.
	 * hopsize:       the number of samples that a window must shift 
	 *                between calculations of sigma.
	 *
	 * The buffer length is given by: 
	 *     (winSize//hopsize//2 +1)*hopsize+initialOffset
	 * where "//" denotes integer division 
	 */
	if (winSize <(hopsize+initialOffset)){
		// should probably check that this actually gets called.
		return -2;
	} else if (winSize % hopsize != 0) {
		return -1;
	}
	return (winSize/hopsize/2 +1)*hopsize+initialOffset;
}


sigOpt *sigOptCreate(int winSize, int hopsize, int initialOffset,
		    int numChannels, float scaleFactor)
{
	if (winSize <= 0) {
		return NULL;
	} else if (hopsize <= 0) {
		return NULL;
	} else if (scaleFactor <=0) {
		return NULL;
	} else if ((initialOffset < 0) || (initialOffset>=winSize)) {
		return NULL;
	} else if (numChannels<=0){
		return NULL;
	}

	int bufferLength = computeSigOptBufferLength(initialOffset, winSize,
						     hopsize);
	if (bufferLength<=0){
		return NULL;
	}
	
	sigOpt *sO = malloc(sizeof(sigOpt));
	sO->winSize = winSize;
	sO->hopsize = hopsize;
	sO->scaleFactor = scaleFactor;
	sO->initialOffset = initialOffset;
	sO->bufferLength = bufferLength;
	sO->numChannels = numChannels;
	sO->bufferTerminationIndex = -1;

	// for now we assume that the location of the window is given by the
	// center of the window
	sO->sizeLeft = winSize/2;
	sO->sizeRight = sO->sizeLeft + 1;
	if (winSize % 2 == 0){
		(sO->sizeLeft)-=1;
	}
	if ((sO->sizeRight + sO->initialOffset)> sO->bufferLength){
		return NULL;
	}

	(sO ->channels) = malloc(sizeof(sigOptChannel)*numChannels);

	int initial_counter = numZeroLeftEdgeAdvancements(hopsize,
							  sO->sizeLeft,
							  initialOffset);
	
	for (int i=0; i<numChannels; i++){
		(sO->channels)[i].windowLeft = bufferIndexCreate(0, 0,
								 bufferLength);
		(sO->channels)[i].windowRight = bufferIndexCreate(0, 0,
								  bufferLength);
		(sO->channels)[i].nobs = 0;
		(sO->channels)[i].ssqdm_x = 0;
		(sO->channels)[i].mean_x = 0;
		(sO->channels)[i].leftEdgeCounter = initial_counter;
	}
	return sO;
}

void sigOptDestroy(sigOpt *sO)
{
	for (int i=0; i<sO->numChannels; i++){
		bufferIndexDestroy((sO->channels)[i].windowLeft);
		bufferIndexDestroy((sO->channels)[i].windowRight);
	}
	free((sO->channels));
	free(sO);
}

int sigOptGetBufferLength(sigOpt *sO){
	return sO->bufferLength;
}

int sigOptGetSigmasPerBuffer(sigOpt *sO){
	return sO->bufferLength/sO->hopsize;
}

int sigOptAdvanceBuffer(sigOpt *sO)
{
	for (int i = 0; i<(sO->numChannels); i++){
		bufferIndexAdvanceBuffer((sO->channels)[i].windowLeft);
		bufferIndexAdvanceBuffer((sO->channels)[i].windowRight);
	}
}

/* the following draws HEAVY inspiration from implementation of pandas 
 * roll_variance. 
 * https://github.com/pandas-dev/pandas/blob/master/pandas/_libs/window.pyx
 * The next 3 functions have been copied from simpleDetFunc.c
 */

static inline double calc_var(double nobs, double ssqdm_x)
{
	double result;
	/* Variance is unchanged if no observation is added or removed
	 */
	if (nobs == 1){
		result = 0;
	} else {
		result = ssqdm_x / (nobs - 1);
		if (result < 0){
			result = 0;
		}
	}
	return result;
}

static inline void add_var(double val, int *nobs, double *mean_x,
			   double *ssqdm_x)
{
	/* add a value from the var calc */
	double delta;

	(*nobs) += 1;
	delta = (val - *mean_x);
	(*mean_x) += delta / *nobs;
	(*ssqdm_x) += (((*nobs-1) * delta * delta) / *nobs);
}

static inline void remove_var(double val, int *nobs, double *mean_x,
			      double *ssqdm_x)
{
	/* remove a value from the var calc */

	double delta;

	(*nobs) -= 1;
	if (*nobs>0){
		delta = (val - *mean_x);
		(*mean_x) -= delta / *nobs;
		(*ssqdm_x) -= (((*nobs+1) * delta * delta) / *nobs);
	} else {
		*mean_x = 0;
		*ssqdm_x = 0;
	}
}

int advanceLeftEdgeHelper(bufferIndex *leftEdge, bufferIndex *stopIndex,
			  int *nobs, double *mean_x, double *ssqdm_x,
			  float *firstArray, float *secondArray)
{
	/* first array must always be non-null. startIndex is always assumed to
	 * be part of the first array.
	 * second array is only ever expected if the buffer num of start and
	 * stop index are different
	 */
	int start_buffer_num = bufferIndexGetBufferNum(leftEdge);
	//int stop_buffer_num = bufferIndexGetBufferNum(stopIndex);

	// this could be updated so that we are not incrementing startIndex
	/* First we would check to see if startIndex and stopIndex have the 
	 * same buffer num. If so, then we just temporary variables to 
	 * represent the indices and increment the indices. At the end, we 
	 * update the value of the leftEdge.
	 * If leftEdge and stopIndex do not share the same buffer num, then we 
	 * use temporary variables to increment left edge to the termination of 
	 * the first array and then use temporary variables to increment the 
	 * start of the second arry to the stop index.
	 */

	for (leftEdge; bufferIndexLt(leftEdge,stopIndex);
	     bufferIndexIncrement(leftEdge)) {
		double val;
		int buffer_num = bufferIndexGetBufferNum(leftEdge);
		int i = bufferIndexGetIndex(leftEdge);
		if (buffer_num == start_buffer_num){
			val = firstArray[i];
		} else {
			val = secondArray[i];
		}
		remove_var(val, nobs, mean_x,ssqdm_x);
	}
	return 1;
}

int advanceLeftEdge(bufferIndex *leftEdge, bufferIndex *stopIndex, int *nobs,
		    double *mean_x, double *ssqdm_x, float *trailingBuffer,
		    float *centralBuffer)
{
	int bufferNum = bufferIndexGetBufferNum(leftEdge);
	int result = 0;
	if (bufferNum == -1){
		result = advanceLeftEdgeHelper(leftEdge, stopIndex, nobs,
					       mean_x, ssqdm_x, trailingBuffer,
					       centralBuffer);
	} else if (bufferNum == 0) {
		result = advanceLeftEdgeHelper(leftEdge, stopIndex, nobs,
					       mean_x, ssqdm_x, centralBuffer,
					       NULL);
	}
	return result;
}

int advanceRightEdgeHelper(bufferIndex *rightEdge, bufferIndex *stopIndex,
			   int *nobs, double *mean_x, double *ssqdm_x,
			   float *firstArray, float *secondArray)
{
	/* first array must always be non-null. startIndex is always assumed to
	 * be part of the first array.
	 * second array is only ever expected if the buffer num of start and
	 * stop index are different
	 */

	int start_buffer_num = bufferIndexGetBufferNum(rightEdge);
	// Again, we can update this so we are not always incrementing rightEdge
	for (rightEdge; bufferIndexLt(rightEdge,stopIndex);
	     bufferIndexIncrement(rightEdge)) {
		double val;
		int buffer_num = bufferIndexGetBufferNum(rightEdge);
		int i = bufferIndexGetIndex(rightEdge);
		if (buffer_num == start_buffer_num){
			val = firstArray[i];
		} else {
			val = secondArray[i];
		}
		add_var(val, nobs, mean_x, ssqdm_x);
	}
	return 1;
}

int advanceRightEdge(bufferIndex *rightEdge, bufferIndex *stopIndex, int *nobs,
		     double *mean_x, double *ssqdm_x, float *centralBuffer,
		     float *leadingBuffer)
{
	int bufferNum = bufferIndexGetBufferNum(rightEdge);
	int result = 0;
	if (bufferNum == 0) {
		result = advanceRightEdge(rightEdge, stopIndex, nobs, mean_x,
					  ssqdm_x, centralBuffer,leadingBuffer);
	} else if (bufferNum == 1){
		result = advanceRightEdge(rightEdge, stopIndex, nobs, mean_x,
					  ssqdm_x, leadingBuffer, NULL);
	} else if ((bufferNum == 2) &&
		   (bufferIndexNe(rightEdge,stopIndex))) {
		result = advanceRightEdge(rightEdge, stopIndex, nobs, mean_x,
					  ssqdm_x, leadingBuffer, NULL);
	}
	return result;
}

int sigOptSetup(sigOpt *sO, int channel, float *buffer)
{
	/* this should only be called once per channel before calling
	 * sigOptAdvance basically this sets up the window, so calling it
	 * sigOptAdvance once will lead the buffer to be centered on the very
	 * first calculation index of the stream.
	 *
	 * basically, this means we want the right edge to be located at
	 * winSize - initialOffset - hopsize at the end of this function.
	 *
	 * NOTE: if we alter the requirements such that winSize can be less 
	 * than (initialOffset + hopsize) then the behavior of the function 
	 * must be modified. 
	 */
	if ((channel>=(sO->numChannels)) || channel<0){
		return -1;
	}
	double mean_x = 0, ssqdm_x = 0;
	int nobs=0;

	int window_start = (sO->initialOffset - sO->sizeLeft - sO->hopsize);
	if ((window_start + sO->hopsize) > 0){
		// this would mean that the very first window is so far to the
		// right that the an entire window is included - this is
		// unintended
		return -1;
	}

	int window_stop = (sO->initialOffset) + (sO->sizeRight) - sO->hopsize;
	if (sO->bufferTerminationIndex!=-1){
		if (window_stop > sO->bufferTerminationIndex){
			window_stop = sO->bufferTerminationIndex;
		}
	}

	for (int i=0; i++; i<window_stop){
		add_var(buffer[i], &nobs, &mean_x, &ssqdm_x);
		// this should be optimized - inefficient to increment
		// bufferIndex
		bufferIndexIncrement((sO->channels)[channel].windowRight);
	}
	(sO->channels)[channel].nobs = nobs;
	(sO->channels)[channel].mean_x = mean_x;
	(sO->channels)[channel].ssqdm_x = ssqdm_x;
	return 1;
}

void terminationRightEdgeStop(int termination_index, bufferIndex *stopIndex,
			      float *centralBuffer, float *leadingBuffer)
{
	/* This adjusts the stopIndex of the right edge if it is past the 
	 * termination index of the stream.
	 */
	if (termination_index <0){
		return;
	}
	int stopBufferNum = bufferIndexGetBufferNum(stopIndex);
	int stopIndexVal = bufferIndexGetIndex(stopIndex);
	int bufferLength = bufferIndexGetBufferLength(stopIndex);

	int terminationBuffer;
	if (leadingBuffer != NULL) {
		// if leadingBuffer is not NULL then the stream terminates in
		// the leading buffer.
		if (stopBufferNum < 1) {
			return;
		}
		terminationBuffer = 1;
	} else {
		terminationBuffer = 0;
	}

	if (stopIndexVal > termination_index) {
		// we just need to adjust the stopIndex to be equal to the
		// termination_index
		if (termination_index == bufferLength) {
			bufferIndexModifyVal(stopIndex, terminationBuffer + 1,
					     0);
		} else {
			bufferIndexModifyVal(stopIndex, terminationBuffer,
					     termination_index);
		}
	}
}

bufferIndex *computeRightStopEdge(int hopsize, int termination_index,
				  bufferIndex *rightEdge, float *centralBuffer,
				  float *leadingBuffer)
{
	bufferIndex *stopIndex = bufferIndexAddScalarIndex(rightEdge, hopsize);
	/* make sure that the right edge is not past the termination of the 
	 * stream. */ 
	terminationRightEdgeStop(termination_index, stopIndex,
				 centralBuffer, leadingBuffer);
	return stopIndex;
}

bufferIndex *firstNonZLeftEdge(int hopsize, int initialOffset, int sizeLeft,
			       bufferIndex *leftEdge)
{
	// not totally sure we are computing this correctly
	// an edge case here is if initialOffset is 0. Then the left edge may
	// actually be equal to 0. With the way that
	// numZeroLeftEdgeAdvancements is written, there could be problems if
	// bufferLength == sizeLeft+hopsize.
	// If this turns out to be a problem, we could redefine this as the
	// first non-negative Left edge - this would alleviate any problems

        /* leftEdge is always equal to the very first index in the stream.
	 *
	 * We start by computing finalCenterLoc for the current advancement - 
	 * which is defined as the center of window at the end of the current 
	 * advancement. By definition, this is the location where leftEdge has 
	 * a non-zero value.
	 */
	int n_adv = numZeroLeftEdgeAdvancements(hopsize, sizeLeft,
						initialOffset);
	/* sigOptAdvanceWindow was called n_adv times before this function was 
	 * called. This function is called during the currently (n_adv+1) call 
	 * of sigOptAdvanceWindow. The first time sigOptAdvanceWindow was 
	 * called the window advanced so that it was centered at initialOffset.
	 * This means that the center of the window at the end of this current 
	 * advancement will be:
	 */
	int finalCenterLoc = initialOffset + hopsize*n_adv;
	return bufferIndexAddScalarIndex(leftEdge, finalCenterLoc-sizeLeft);
}

float sigOptAdvanceWindow(sigOpt *sO, float *trailingBuffer,
			  float *centralBuffer, float *leadingBuffer,
			  int channel)
{
	/* This function always adds hopsize to the left edge and right edge 
	 * (unless the right edge already up against the termination edge).
	 *
	 * After making the advancement, it returns the value of std.
	 *
	 * this function always expects a non-NULL value for centralBuffer. It 
	 * is possible for trailingBuffer and leadingBuffer to be NULL (at the 
	 * start and ends of the stream).
	 * 
	 * the function returns -1 if an erro is encountered
	 *
	 * NOTE: if we alter the requirements such that winSize can be less 
	 * than (initialOffset + hopsize) then the behavior of the function 
	 * must be modified. 
	 */
	int hopsize = sO->hopsize;
	int termination_index = sO->bufferTerminationIndex;
	int nobs = (sO->channels)[channel].nobs;
	double mean_x = (sO->channels)[channel].mean_x;
	double ssqdm_x = (sO->channels)[channel].ssqdm_x;
	bufferIndex *leftEdge = (sO->channels)[channel].windowRight;
	bufferIndex *rightEdge = (sO->channels)[channel].windowRight;
	bufferIndex *stopIndex;
	int result;

	// I think there is a slight edge case here - If total stream length,
	// streamLength, satisfies winSize/2< streamLength <winSize
	// specifically think about if the initialOffset = 22 and
	// streamLength = winSize/2 + 21
	// if this is a problem, the way to solve it would be to add a counter
	// to each sigOptChannel. Then increment it every time that we advance
	// the buffer while nobs<0 and left edge has not moved. After a certain
	// number of increments, you will know to advance the left edge. Then,
	// you can completely disregard this counter.
	if (nobs < (sO->winSize)){
		if ((sO->channels)[channel].leftEdgeCounter >= 0){
			/* The leftEdge of the window is at zero */
			if ((sO->channels)[channel].leftEdgeCounter == 0){
				/* advance the left edge */
				stopIndex = firstNonZLeftEdge(hopsize,
							      sO->initialOffset,
							      sO->sizeLeft,
							      leftEdge);
				result = advanceLeftEdge(leftEdge, stopIndex,
							 &nobs, &mean_x,
							 &ssqdm_x,
							 centralBuffer, NULL);
				bufferIndexDestroy(stopIndex);
				if (result == 0){
					return 0;
				}
			}

			(sO->channels)[channel].leftEdgeCounter--;

			/* only advance the right edge to 
			 * min(right_edge + hopsize, termination_index)
			 * this minimization is accounted for in 
			 * computeRightStopEdge
			 * If I call advanceRightEdge, and 
			 * rightEdge == stopEdge, then the window does not
			 * get advanced - we just introduce extra overhead
			 */
			stopIndex = computeRightStopEdge(hopsize,
							 termination_index,
							 rightEdge,
							 centralBuffer,
							 leadingBuffer);
			result = advanceRightEdge(rightEdge, stopIndex, &nobs,
						  &mean_x, &ssqdm_x,
						  centralBuffer, leadingBuffer);
		} else {
			/* In this case the right edge is up against the 
			 * termination index. Thus we only advance the left
			 * edge. We handle this case separately from below to 
			 * avoid unescesary overhead of calling 
			 * advanceRightEdge (however it would know not to 
			 * advance the right edge).
			 */
			stopIndex = bufferIndexAddScalarIndex(leftEdge,
							      hopsize);
			result = advanceLeftEdge(leftEdge, stopIndex, &nobs,
						 &mean_x, &ssqdm_x,
						 trailingBuffer, centralBuffer);
		}
	} else {
		/* first lets advance the left edge. */
		stopIndex = bufferIndexAddScalarIndex(leftEdge, hopsize);
		result = advanceLeftEdge(leftEdge, stopIndex, &nobs, &mean_x,
					 &ssqdm_x, trailingBuffer,
					 centralBuffer);
		bufferIndexDestroy(stopIndex);
		if (result == 0){
			return 0;
		}

		/* Next lets advance the right edge. */
		stopIndex = computeRightStopEdge(hopsize, termination_index,
						 rightEdge, centralBuffer,
						 leadingBuffer);
		result = advanceRightEdge(rightEdge, stopIndex, &nobs, &mean_x,
					  &ssqdm_x, centralBuffer,
					  leadingBuffer);
	}
	bufferIndexDestroy(stopIndex);

	if (result == 0){
		return 0;
	}

	(sO->channels)[channel].nobs = nobs;
	(sO->channels)[channel].mean_x = mean_x;
	(sO->channels)[channel].ssqdm_x = ssqdm_x;
	return sigOptGetSigma(sO, channel);
}

float sigOptGetSigma(sigOpt *sO, int channel)
{
	if ((sO->channels)[channel].nobs <= 1){
		return -1;
	}
	float std = sqrtf((float)calc_var((sO->channels)[channel].nobs,
					  (sO->channels)[channel].ssqdm_x));

	return (sO->scaleFactor) * std / powf((sO->channels)[channel].nobs,
					      0.2);
}

int sigOptSetTerminationIndex(sigOpt *sO,int index)
{
	if (sO->bufferTerminationIndex != -1){
		return -1;
	} else if (index < 0) {
		return -2;
	} else if (index >= sO->bufferLength) {
		return -3;
	} else {
		sO->bufferTerminationIndex=index;
		return 1;
	}
}


int sigOptFullRollSigma(int initialOffset, int hopsize, float scaleFactor,
			int winSize, int dataLength, int numWindows,
			float *input, float **sigma)
{
	sigOpt *sO = sigOptCreate(winSize, hopsize, initialOffset, 1,
				  scaleFactor);
	if (sO == NULL){
		return -1;
	}

	int bufferLength = sigOptGetBufferLength(sO);
	int sigPerBuffer = sigOptGetSigmasPerBuffer(sO);
	int niter = numWindows/sigPerBuffer;
	int termination_index = (numWindows % sigPerBuffer);

	if (termination_index!=0){
		niter++;
	}

	float *trailingBuffer = NULL;
	// set the central buffer equal to the very start of the input
	float* centralBuffer = input;

	int result;
	// setup sigOpt with the first buffer
	result = sigOptSetup(sO, 0, centralBuffer);
	if (result <1){
		sigOptDestroy(sO);
		return -1;
	}

	// I am not sure this will work for short enough arrays

	float *leadingBuffer = input + bufferLength;

	int k=0;
	for (int i=0;i<(niter-2);i++){		
		for (int j=0;j<sigPerBuffer;j++){
			float sig = sigOptAdvanceWindow(sO, trailingBuffer,
							centralBuffer,
							leadingBuffer, 0);
			if (sig<0){
				sigOptDestroy(sO);
				return -1;
			}
			(*sigma)[k] = sig;
			k++;
		}
		trailingBuffer = centralBuffer;
		centralBuffer = leadingBuffer;
		leadingBuffer = input + ((i+1)*bufferLength);
	}

	// now set the termination index
	sigOptSetTerminationIndex(sO,termination_index);
	// now calculate the second to last one
	for (int j=0;j<sigPerBuffer;j++){
		float sig = sigOptAdvanceWindow(sO, trailingBuffer,
						centralBuffer,leadingBuffer,0);
		if (sig<0){
			sigOptDestroy(sO);
			return -1;
		}
		(*sigma)[k] = sig;
		k++;
	}
	trailingBuffer = centralBuffer;
	centralBuffer = leadingBuffer;
	leadingBuffer = NULL;

	// now calculate the remaining values.
	for (k; k<numWindows; k++){
		float sig = sigOptAdvanceWindow(sO, trailingBuffer,
						centralBuffer, leadingBuffer,0);
		if (sig<0){
			sigOptDestroy(sO);
			return -1;
		}
		(*sigma)[k] = sig;
	}

	// finally, clean up.
	sigOptDestroy(sO);
	return 1;
}
