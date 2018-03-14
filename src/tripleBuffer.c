#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "tripleBuffer.h"

struct tripleBuffer
{
	int num_channels;
	int buffer_length;
	float **buffers;
	int termination_index;
	int num_buffers;
	int first_buffer;
};

tripleBuffer *tripleBufferCreate(int num_channels, int buffer_length)
{
	if ((num_channels <= 0) || (buffer_length <= 0)) {
		return NULL;
	} else if ((INT_MAX / buffer_length) < num_channels) {
		return NULL;
	}
	
	tripleBuffer *tB = malloc(sizeof(tripleBuffer));

	if (tB == NULL)
	{
		return NULL;
	}

	tB->num_channels = num_channels;
	tB->buffer_length = buffer_length;
	tB->num_buffers = 0;

	tB->buffers = malloc(sizeof(float*)*3);
	tB->first_buffer = -1;
	tB->termination_index = -1;

	for (int i=0; i<3; i++){
		(tB->buffers)[i] = malloc(sizeof(float) * num_channels *
	        			  buffer_length);
	}

	// need to allocate buffers - it should be set to an array of 3 float
	// pointers
	// need to initialize trailing_index - probably to -1
	// need to initialize num_buffers to 0
	return tB;
}

void tripleBufferDestroy(tripleBuffer *tB)
{
	// need to worry about freeing buffers and its contents

	for (int i=0; i<3; i++){
		free(tB->buffers[i]);
	}
	free(tB->buffers);
		     
	free(tB);
}

int tripleBufferNumChannels(tripleBuffer *tB)
{
	return tB->num_channels;
}

int tripleBufferBufferLength(tripleBuffer *tB)
{
	return tB->buffer_length;
}

int tripleBufferNumBuffers(tripleBuffer *tB)
{
	return tB->num_buffers;
}

int tripleBufferAddLeadingBuffer(tripleBuffer *tB)
{
	if (tB->termination_index!=-1){
		return 0;
	}
	// write a test for when termination_index is not 0
	if (tB->first_buffer == -1){
		tB->first_buffer = 0;
	}	
	if (tB->num_buffers < 3){
		tB->num_buffers++;
	} else {
		return 0;
	}
	return 1;
}

void swapPtrArrayEntries(float **array, int index1, int index2){
	float *temp = array[index1];
	array[index1] = array[index2];
	array[index2] = temp;
}

int tripleBufferRemoveTrailingBuffer(tripleBuffer *tB)
{
	// this can be simplified if we require that the stream be terminated
	// for this function to work. The fact that I want to be able to still
	// add buffers and cycle buffers afterwards require that we be more
	// careful.
	int num_buffers = tripleBufferNumBuffers(tB);
	int first_buffer = tB->first_buffer;
		
	if  (num_buffers == 0){
		return 0;
	} else if (num_buffers == 1) {
		tB->num_buffers = 0;
		tB->first_buffer = -1;
	} else if (num_buffers == 2) {
		tB->num_buffers = 1;
		int index2;
		if (first_buffer == 0) {
			index2 = 1;
		} else if (first_buffer == 1) {
			index2 = 2;
		} // in the third case there is no need to do any swapping
		swapPtrArrayEntries(tB->buffers, 0, index2);
	} else {
		tB->num_buffers = 2;
		if (first_buffer == 0) {
			swapPtrArrayEntries(tB->buffers, 0, 1);
			swapPtrArrayEntries(tB->buffers, 1, 2);
		} else if (first_buffer == 1){
			// only need 1 swap
			swapPtrArrayEntries(tB->buffers, 0, 2);
		} // again, the third case requires no swapping
	}

	return 1;
}

float *tripleBufferGetBufferPtr(tripleBuffer *tB, int bufferIndex,
				int channelNum)
{
	if ((channelNum<0) || (channelNum>=tripleBufferNumChannels(tB))){
		return NULL;
	} else if (bufferIndex >= tripleBufferNumBuffers(tB)) {
		return NULL;
	} else if (tripleBufferNumBuffers(tB) == 0) {
		return NULL;
	}

	int offset = channelNum * tripleBufferBufferLength(tB);
	int index = (bufferIndex + tB->first_buffer) % (tB->num_buffers);
	return ((tB->buffers)[index])+offset;

}

int tripleBufferCycle(tripleBuffer *tB)
{
	// write a test for when the stream is terminated
	// write a test for when a buffer is removed
	// write a test for when a buffer is removed that is not contiguous
	if ((tB->termination_index != -1) || (tB->num_buffers==0)) {
		return 0;
	}
	tB->first_buffer = (tB->first_buffer+1);
	if ((tB->first_buffer) == (tB->num_buffers)){
		tB->first_buffer = 0;
	}
	return 1;
}

int tripleBufferIsTerminatedStream(tripleBuffer *tB){
	if (tB->termination_index != -1) {
		return 1;
	} else {
		return 0;
	}
}

int tripleBufferGetTerminalIndex(tripleBuffer *tB){
	return tB->termination_index;
}

int tripleBufferSetTerminalIndex(tripleBuffer *tB, int terminalBufferIndex)
{
	if ((tripleBufferNumBuffers(tB) == 0) ||
	    (terminalBufferIndex < 0) ||
	    (terminalBufferIndex >= tripleBufferBufferLength(tB)) ||
	    (tB->termination_index!=-1)){
		return 0;
	}
	tB->termination_index = terminalBufferIndex;
	return 1;
}
