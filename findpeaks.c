#include <math.h>
#include <stdlib.h>
#include "peakQueue.h"
#include "findpeaks.h"



int findpeaks(double* x, double* y, long length,double slopeThreshold, 
	      double ampThreshold, double smoothwidth, int peakgroup,
	      int smoothtype, int N, int first, double* peakX,
	      double* peakY, double* firstPeakX)
{
	// This function has been transcribed from T. C. O'Haver's findpeaks
	// function.

	// Function to locate the positive peaks in a noisy x-y time series
	// data set.  Detects peaks by looking for downward zero-crossings
	// in the first derivative that exceed SlopeThreshold. Modifies peakX
	// and peakY to contain the position and height of each peak.
	// Arguments "slopeThreshold", "ampThreshold" and "smoothwidth"
	// control peak sensitivity. Higher values will neglect smaller
	// features. "Smoothwidth" is the width of the smooth applied before
	// peak detection; larger values ignore narrow peaks.
	// "peakgroup" is the number points around the top part of the peak
	// that are taken for measurement.
	//  If smoothtype=1, rectangular (sliding-average or boxcar)
	//  If smoothtype=2, triangular (2 passes of sliding-average)
	//  If smoothtype=3, pseudo-Gaussian (3 passes of sliding-average)
	// T. C. O'Haver, 1995.  Version 4,  Last revised September, 2011

	// we will probably only use smooth type = 3

	// There are is a change from the original code. The original code 
	// returned the information about all detected peaks. Our purposes 
	// require that we only return a certain number of peaks. The 
	// maximum number of returned peaks is set by "N". The actual 
	// number of returned peaks returned by the function. If a positive 
	// integer is passed to "first", then the first "N" peaks are returned.
	// Otherwise, the "N": peaks with the highest amplitude are returned.

	// "peakX", "peakY", and will point at the peaks with the greatest 

	smoothwidth = round(smoothwidth);
	peakgroup = round(peakgroup);
	double* d=fastsmooth(deriv(y,length),length, smoothwidth, smoothtype);
	int n = (int)round(peakgroup/2 +1);
	//int outIndex = 0;
	
	long j, temp;
	double curPeakX,curPeakY;

	struct peakQueue peakQ=peakQueueNew(N);

	// Here we try to find the first peak
	for (j=((long)smoothwidth)-1;j<(length-(long)smoothwidth);j++){
		// check for zero-crossing
		if (sign(d[j]) > sign(d[j+1])) {
			// check if the slope of the derivative is greater than 
			// slopeThresholdlong* peakIndices, 
			if ((d[j]-d[j+1])>(slopeThreshold*y[j])) {
				// check if the height of the peak exceeds
				// ampThreshold
				if (y[j]>ampThreshold) {
					findpeaksHelper(x, y, length, peakgroup,
							&curPeakX, &curPeakY,
							j,n);

					peakQueueAddNewPeak(&peakQ, curPeakX,
							    curPeakY);
					*firstPeakX = curPeakX;

					if ((first>=1)&(peakQ.cur_size == N) ){
						j = length;
					}
					break;
				}
			}
		}
	}
	
	temp = j;

	// Here, we find the remaining peaks
	for (j=temp;j<(length-(long)smoothwidth);j++){
		// check for zero-crossing
		if (sign(d[j]) > sign(d[j+1])) {
			// check if the slope of the derivative is greater than 
			// slopeThresholdlong* peakIndices, 
			if ((d[j]-d[j+1])>(slopeThreshold*y[j])) {
				// check if the height of the peak exceeds
				// ampThreshold
				if (y[j]>ampThreshold) {
					findpeaksHelper(x, y, length, peakgroup,
							&curPeakX, &curPeakY,
							j,n);

					peakQueueAddNewPeak(&peakQ, curPeakX,
							    curPeakY);

					if ((first>=1)&(peakQ.cur_size == N) ){
						j = length;
					}		
				}
			}
		}
	}
	free(d);

	int out = peakQueueToArrays(&peakQ,peakX,peakY);
	peakQueueDestroy(peakQ);
	return out;
}

int sign(double x)
{
	if (x > 0){
		return 1;
	} else if (x == 0) {
		return 0;
	} else {
		return -1;
	}
}

void findpeaksHelper(double* x, double* y, long length, int peakgroup,
		     double* peakX, double* peakY, long j, int n)
{
	// Helper Function
	int k;
	long groupindex;
	if (peakgroup<5){
		*peakY = -1.0;
		// At a glance the following for loop seems wrong (especiall in
		// comparison to the matlab code but I think its actually
		// correct)
		for (k=1;k<=peakgroup;k++){
			groupindex = (j+(long)k-(long)n+1);
			if (groupindex < 0) {
				groupindex = 0;
			}
			if (groupindex >= length) {
				groupindex = length-1;
			}
			if (y[groupindex]>*peakY) {
				*peakY = y[groupindex];
				*peakX = x[groupindex];
			}
		}
	} else {
		double* xx = malloc(sizeof(double)*peakgroup);
		double* yy = malloc(sizeof(double)*peakgroup);
		double mean, std;
		double *coef;
		// At a glance the following for loop seems wrong (especiall
		// in comparison to the matlab code), but I think its actually
		// correct)
		for (k=1;k<=peakgroup;k++){
			groupindex = (j+(long)k-(long)n+1);
			if (groupindex < 0) {
				groupindex = 0;
			}
			if (groupindex >= length) {
				groupindex = length-1;
			}
			xx[k-1]=x[groupindex];
			yy[k-1]=log(y[groupindex]);
		}
		
		// fit parabola to log10 of sub-group with centering and scaling
		coef = quadFit(xx, yy, peakgroup, &mean, &std);
		*peakX = -((std*coef[1]/(2.*coef[2]))-mean);
		// check that we are correctly squaring
		*peakY = exp(coef[0]-coef[2]*pow((coef[1]/(2.*coef[2])),2));
	}
}

double* quadFit(double* x, double* y, long length, double* mean, double *std)
{
	// Fits a Quadratic to data using least square fitting, like matlab's
	// polyfit function
	// https://www.mathworks.com/help/matlab/ref/polyfit.html
	
	// Like polyfit, we center x at zero and scale it to have 1 unit
	// standard deviation
	// xp = (x-mean)/std
	// Here mean is the average x value and std is the stamdard deviation

	double* coef = malloc(sizeof(double)*3);
	long i;
	for (*mean=0.0,i=0;i<length;i++){
		*mean+=x[i];
	}
	*mean/=(double)length;

	double temp;
	// assuming that all values of x are real
	for (*std=0.0,i=0;i<length;i++){
		temp=(x[i]-*mean);
		*std+=temp*temp;
	}
	*std = sqrt(*std/((double)(length-1)));

	// now determine xp

	double* xp = malloc(sizeof(double)*length);
	for (i=0;i<length;i++){
		xp[i]=(x[i]-*mean) / *std;
	}

	// fit the actual polynomial to xp and y, we a method equivalent 
	// to the one used by matlab. Instructions for that kind of fitting 
	// can be found at:
	// http://mathworld.wolfram.com/LeastSquaresFittingPolynomial.html

	// The actual fitting algorithm we use are described at 
	// https://math.stackexchange.com/questions/267865/equations-for-quadratic-regression
	// The only differences are in the varaible names:
	// S_11 -> sxx
	// S_12 -> sxx2
	// S_22 -> sx2x2
	// S_y1 -> sxy
	// S_y2 -> sx2y
	double sxx, sxx2, sx2x2, sxy, sx2y, sumx, sumx2, sumy, cur_x, cur_x2;
	double cur_y;

	for (i=0;i<length;i++){
		cur_x = xp[i];
		cur_x2 = cur_x * cur_x;
		cur_y = y[i];
		sumx += cur_x;
		sumx2 += cur_x2;
		sumy += cur_y;
		sxx2 += (cur_x2 * cur_x);
		sx2x2 += (cur_x2*cur_x2);
		sxy += (cur_x * cur_y);
		sx2y += (cur_x2 * cur_y);
	}

	sxx = sumx2 - ((sumx*sumx)/((double)length));
	sxx2 -= ((sumx*sumx2)/((double)length));
	sx2x2 -= ((sumx2*sumx2)/((double)length));
	sxy -= ((sumx*sumy)/((double)length));
	sx2y -= ((sumx2*sumy)/((double)length));

	coef[2] = ((sx2y * sxx)-(sxy * sxx2))/((sxx * sx2x2)-(sxx2 * sxx2));
	coef[1] = ((sx2y * sxx)-(sxy * sxx2))/((sxx * sx2x2)-(sxx2 * sxx2));
	coef[0] = (sumy - (coef[1]*sumx)-(coef[2]*sumx2))/((double)length);

	free(xp);
	return coef;
}


double* deriv(double* a, long length)
{
	// First derivative of vector using 2-point central difference.
	// Transcribed from matlab code of T. C. O'Haver, 1988.
	// I am very confident that I transcribed the indices of the 
	// arrays properly
	long i;
	double* d = malloc( sizeof(double) * length);
	for(i=0;i<length-1;i++){
		d[i] = (d[i+1]-d[i])/2.;
	}
	d[length-1] = d[length-2];
	return d;
}

double* fastsmooth(double* y, long length, double w, int type)
{
	// Transcribed from matlab code of T. C. O'Haver, 1988 Version 2.0, 
	// May 2008.
	// smooths array with smooth of width w.
	//  If type=1, rectangular (sliding-average or boxcar)
	//  If type=2, triangular (2 passes of sliding-average)
	//  If type=3, pseudo-Gaussian (3 passes of sliding-average)
	double* smoothY = malloc(sizeof(double)*length);
	switch (type){
	case 1 :
		smoothY = sa(y,length,w);
		break;
	case 2 :
		smoothY = sa(sa(y,length,w),length,w);
		break;
	case 3 :
		smoothY = sa(sa(sa(y,length,w),length,w),length,w);
		break;
	}
	return smoothY;
}

double* sa(double* y, long length, double smoothwidth)
{
	// should probably check that smooth width is>=1

	// I am fairly confident that I accurately transcribed 
	// the indices accurately from matlab to c for this 
	// function. 

	// I am a little uncertain about matlab's round 
	// function and if I have imitated its behavior - the 
	// uncertainty boils down to whether or not matlab's 
	// round function returns a int data type. If that is 
	// the case, then this function is wrong.
	long w = (long)round(smoothwidth);
	double sumPoints = 0;
	long k;
	for(k=0;k<w;k++){
		sumPoints += y[k];
	}

	// create s which is an array of zeros
	double* s = malloc(sizeof(double)*length);
	for(k=0;k<length;k++){
		s[k]=0;
	}

	long halfw = (long)round(((double)w)/2.);
	for(k=0;k<length-w;k++){
		s[k+halfw-1]=sumPoints/((double)w);
		sumPoints = sumPoints-y[k]+y[k+w];
	}

	k = length - w - 1 + halfw;
	long i;
	for (i=(length-w);i<length;i++){
		s[k] += y[i];
	}
	s[k]/=((double)w);
	return s;
}
