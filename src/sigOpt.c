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
	// if (bI->index == INT_MAX){
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
	int variable; /* expects 1 or 0 */
	int winSize;
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
};

sigOpt *sigOptCreate(int variable, int winSize, int hopsize, int startIndex,
		     int bufferLength, int numChannels, float scaleFactor)
{
	if (variable != 1){
		// for now this must be 1. In principal it could be 0.
		return NULL;
	} else if (winSize <= 0) {
		return NULL;
	} else if (hopsize <= 0) {
		return NULL;
	} else if (scaleFactor <=0) {
		return NULL;
	} else if ((startIndex < 0) || (startIndex>=winSize)) {
		return NULL;
	} else if (bufferLength<=0){
		return NULL;
	} else if (numChannels<=0){
		return NULL;
	}
	sigOpt *sO = malloc(sizeof(sigOpt));
	sO->variable = variable;
	sO->winSize = winSize;
	sO->hopsize = hopsize;
	sO->scaleFactor = scaleFactor;
	sO->initialOffset = startIndex;
	sO->bufferLength = bufferLength;
	sO->numChannels = numChannels;
	sO->bufferTerminationIndex = -1;
	(sO ->channels) = malloc(sizeof(sigOptChannel)*numChannels);

	for (int i=0; i<numChannels; i++){
		(sO->channels)[i].windowLeft = bufferIndexCreate(0, 0,
								 bufferLength);
		(sO->channels)[i].windowRight = bufferIndexCreate(0, 0,
								  bufferLength);
		(sO->channels)[i].nobs = 0;
		(sO->channels)[i].ssqdm_x = 0;
		(sO->channels)[i].mean_x = 0;
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

/* the following draws HEAVY inspiration from implementation of pandas 
 * roll_variance. 
 * https://github.com/pandas-dev/pandas/blob/master/pandas/_libs/window.pyx
 * The next 3 functions have been copied from simpleDetFunc.c
 */

static inline double calc_var(double nobs, double ssqdm_x){
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
			   double *ssqdm_x){
	/* add a value from the var calc */
	double delta;

	(*nobs) += 1;
	delta = (val - *mean_x);
	(*mean_x) += delta / *nobs;
	(*ssqdm_x) += (((*nobs-1) * delta * delta) / *nobs);
}

static inline void remove_var(double val, int *nobs, double *mean_x,
			      double *ssqdm_x){
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

float sigOptGetSigma(sigOpt *sO, int channel){
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
