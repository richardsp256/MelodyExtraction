#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "tripleBuffer.h"

struct tripleBuffer
{
	int num_channels;
	int buffer_length;
	float **buffers;
	int trailing_index;
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

void tripleBufferAddLeadingBuffer(tripleBuffer *tB)
{
	if (tB->first_buffer == -1){
		tB->first_buffer = 0;
	}
	if (tB->num_buffers < 3){
		tB->num_buffers++;
	}
	return;
}

int tripleBufferGetBufferIndexHelper(tripleBuffer *tB, int bufferIndex){
	//if (tB->num_buffers == 1){
	//	if (bufferIndex == 0) {
	//		// sanity check - this needs to happen
	//		return tB->first_buffer;
	//	}
	//} else if (tB->num_buffers == 2) {
	//	
	//} else {
	//}

	return (bufferIndex + tB->first_buffer) % (tB->num_buffers);
}

float *tripleBufferGetBufferPtr(tripleBuffer *tB, int bufferIndex,
				int channelNum){
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
