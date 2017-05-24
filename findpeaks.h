int findpeaks(double* x, double* y, long length,double slopeThreshold, 
	      double ampThreshold, double smoothwidth, int peakgroup,
	      int smoothtype, int N, int first, long* peakIndices,
	      double* peakX, double* peakY, double* measuredWidth);
int sign(double x);
void findpeaksHelper(double* x, double* y, long length, int peakgroup, 
		     long* peakIndex, double* peakX, double* peakY,
		     double* measuredWidth, long j, int n);
double* quadFit(double* x, double* y, long length, double* mean, double *std);
double* deriv(double* a, long length);
double* fastsmooth(double* y, long length, double w, int type);
double* sa(double* y, long length, double smoothwidth);
