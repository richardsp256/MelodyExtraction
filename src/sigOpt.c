#include <stdio.h>
#include <stdlib.h>
#include "sigOpt.h"

struct sigOptChannel{
	int variable; /* expects 1 or 0 */
	int sizeLeft;
	int sizeRight;
	int hopsize;
	int leftOffset; // The number of indices from the left of the start of
	                // a calculaton interval which is the center of the
	                // window.
	int curLoc;
	int numTrailingIntervals; // this is the number of intervals where the
	                          // left index of the window refers to an
	                          // index of the trailing buffer
	int centralBufferStart; // where the left most index should be for
	                        // computing the interval just after the left
	                        // window edge was moved to the central buffer 
	int rightBufferStart; // where the right most index should be for the 
	                      // first interval after starting a new buffer
	int numBuffers;
	int bufferLength;
	int bufferTerminationIndex;

	int nobs;
	double mean_x;
	double ssqdm_x;

	float scaleFactor;
};

sigOptChannel *sigOptChannelCreate(int variable, int winSize, int hopsize,
				   int startIndex, int leftOffset,
				   int bufferLength, float scaleFactor)
{
	sigOptChannel *sOC = malloc(sizeof(sigOptChannel));
	return sOC;
}

void sigOptChannelDestroy(sigOptChannel *sOC)
{
	free(sOC);
}
