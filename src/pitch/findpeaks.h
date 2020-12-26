#include <stddef.h> // size_t

#ifndef FINDPEAKS_H
#define FINDPEAKS_H
struct peak {
	float peakX;
	float peakY;
};

struct peakQueue{
	struct peak* array;
	int max_size;
	int cur_size;
};

#endif /*FINDPEAKS_H*/

int findpeaks(const float* x, const float* y, long length,float slopeThreshold, 
	      float ampThreshold, float smoothwidth, int peakgroup,
	      int smoothtype, int N, bool first, float* peakX, float* peakY,
	      float* firstPeakX);

float* quadFit(float* x, float* y, long length, float* mean, float *std);
float* deriv(const float* a, size_t length);
float* fastsmooth(float* y, long length, float w, int type);
float* sa(const float* y, long length, float smoothwidth);

struct peakQueue peakQueueNew(int max_size);
void peakQueueDestroy(struct peakQueue peakQ);
void peakQueueSwapPeaks(struct peakQueue *peakQ, int index1, int index2);
void peakQueueBubbleUp(struct peakQueue *peakQ,int index);
void peakQueueBubbleDown(struct peakQueue *peakQ, int index);
void peakQueueInsert(struct peakQueue *peakQ, struct peak newPeak);
struct peak peakQueuePop(struct peakQueue *peakQ);
void peakQueueAddNewPeak(struct peakQueue *peakQ, float peakX, float peakY);
int peakQueueToArrays(struct peakQueue *peakQ, float* peakX, float* peakY);
void peakQueuePrint(struct peakQueue *peakQ);
