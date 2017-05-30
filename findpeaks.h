#ifndef FINDPEAKS_H
#define FINDPEAKS_H
struct peak {
	double peakX;
	double peakY;
};

struct peakQueue{
	struct peak* array;
	int max_size;
	int cur_size;
};

#endif /*FINDPEAKS_H*/

int findpeaks(double* x, double* y, long length,double slopeThreshold, 
	      double ampThreshold, double smoothwidth, int peakgroup,
	      int smoothtype, int N, int first, double* peakX, double* peakY,
	      double* firstPeakX);
int sign(double x);
void findpeaksHelper(double* x, double* y, long length, int peakgroup, 
		     double* peakX, double* peakY, long j, int n);
double* quadFit(double* x, double* y, long length, double* mean, double *std);
double* deriv(double* a, long length);
double* fastsmooth(double* y, long length, double w, int type);
double* sa(double* y, long length, double smoothwidth);

struct peakQueue peakQueueNew(int max_size);
void peakQueueDestroy(struct peakQueue peakQ);
void peakQueueSwapPeaks(struct peakQueue *peakQ, int index1, int index2);
void peakQueueBubbleUp(struct peakQueue *peakQ,int index);
void peakQueueBubbleDown(struct peakQueue *peakQ, int index);
void peakQueueInsert(struct peakQueue *peakQ, struct peak newPeak);
struct peak peakQueuePop(struct peakQueue *peakQ);
void peakQueueAddNewPeak(struct peakQueue *peakQ, double peakX, double peakY);
int peakQueueToArrays(struct peakQueue *peakQ, double* peakX, double* peakY);
void peakQueuePrint(struct peakQueue *peakQ);
