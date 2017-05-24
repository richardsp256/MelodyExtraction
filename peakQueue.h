#ifndef PEAKQUEUE_H
#define PEAKQUEUE_H
struct peak {
	long index;
	double peakX;
	double peakY;
	double measuredWidth;
};

struct peakQueue{
	struct peak* array;
	int max_size;
	int cur_size;
};

#endif /*PEAKQUEUE_H*/

struct peakQueue peakQueueNew(int max_size);
void peakQueueDestroy(struct peakQueue peakQ);
void peakQueueSwapPeaks(struct peakQueue peakQ, int index1, int index2);
void peakQueueBubbleUp(struct peakQueue peakQ,int index);
void peakQueueBubbleDown(struct peakQueue peakQ, int index);
void peakQueueInsert(struct peakQueue peakQ, struct peak newPeak);
struct peak peakQueuePop(struct peakQueue peakQ);
void peakQueueAddNewPeak(struct peakQueue peakQ, long index, double peakX, 
			 double peakY, double measuredWidth);
int peakQueueToArrays(struct peakQueue peakQ,long* peakIndices, double* peakX,
		      double* peakY, double* measuredWidth);
