#ifndef TRIPLEBUFFER_H
#define TRIPLEBUFFER_H
typedef struct tripleBuffer tripleBuffer;

tripleBuffer *tripleBufferCreate(int num_channels, int buffer_length);
void tripleBufferDestroy(tripleBuffer *tB);
int tripleBufferNumChannels(tripleBuffer *tB);
int tripleBufferBufferLength(tripleBuffer *tB);
int tripleBufferNumBuffers(tripleBuffer *tB);
void tripleBufferAddLeadingBuffer(tripleBuffer *tB);
float *tripleBufferGetBufferPtr(tripleBuffer *tB, int bufferIndex,
				int channelNum);

//void tripleBufferRemoveTrailingBuffer(tripleBuffer *tB);
//float *tripleBufferGetLeadingBuffer(tripleBuffer *tB, int channel);
//float *tripleBufferGetCentralBuffer(tripleBuffer *tB, int channel);
//float *tripleBufferGetTrailingBuffer(tripleBuffer *tB, int channel);
//float *tripleBufferCycleBuffers(tripleBuffer *tB);

#endif /* TRIPLEBUFFER_H */
