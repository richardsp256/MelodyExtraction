#ifndef TRIPLEBUFFER_H
#define TRIPLEBUFFER_H
typedef struct tripleBuffer tripleBuffer;

tripleBuffer *tripleBufferCreate(int num_channels, int buffer_length);
void tripleBufferDestroy(tripleBuffer *tB);
int tripleBufferNumChannels(tripleBuffer *tB);
int tripleBufferBufferLength(tripleBuffer *tB);
int tripleBufferNumBuffers(tripleBuffer *tB);
int tripleBufferAddLeadingBuffer(tripleBuffer *tB);
int tripleBufferRemoveTrailingBuffer(tripleBuffer *tB);
float *tripleBufferGetBufferPtr(tripleBuffer *tB, int bufferIndex,
				int channelNum);
int tripleBufferCycle(tripleBuffer *tB);
int tripleBufferIsTerminatedStream(tripleBuffer *tB);
int tripleBufferGetTerminalIndex(tripleBuffer *tB);
int tripleBufferSetTerminalIndex(tripleBuffer *tB, int terminalBufferIndex);

#endif /* TRIPLEBUFFER_H */
