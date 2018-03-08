#include <stdlib.h>
#include <stdio.h>
#include "tripleBuffer.h"

struct tripleBuffer
{
	int num_channels;
	int buffer_length;
	//float **buffers;
	//int trailing_index;
	//int num_buffers;
};

tripleBuffer *tripleBufferCreate(int num_channels, int buffer_length)
{
	tripleBuffer *tB = malloc(sizeof(tripleBuffer));

	if (tB == NULL)
	{
		return NULL;
	}

	tB->num_channels = num_channels;
	tB->buffer_length = buffer_length;

	// need to allocate buffers - it should be set to an array of 3 float
	// pointers
	// need to initialize trailing_index - probably to -1
	// need to initialize num_buffers to 0
	return tB;
}

void tripleBufferDestroy(tripleBuffer *tB)
{
	// need to worry about freeing buffers and its contents
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
