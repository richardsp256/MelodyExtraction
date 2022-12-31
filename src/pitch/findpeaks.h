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

int findpeaks(float* x, float* y, long length,float slopeThreshold, 
	      float ampThreshold, float smoothwidth, int peakgroup,
	      int smoothtype, int N, int first, float* peakX, float* peakY,
	      float* firstPeakX);
int sign(float x);
void findpeaksHelper(float* x, float* y, long length, int peakgroup, 
		     float* peakX, float* peakY, long j, int n);
float* quadFit(float* x, float* y, long length, float* mean, float *std);
float* deriv(float* a, long length);
float* fastsmooth(float* y, long length, float w, int type);
float* sa(float* y, long length, float smoothwidth);

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
