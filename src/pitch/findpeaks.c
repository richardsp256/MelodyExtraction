/*

Copyright (c) 2016 Thomas C. O'Haver
 
Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sub-license, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "findpeaks.h"

// it makes slightly more sense to implement this function after findpeaks
static void findpeaksHelper(const float* x, const float* y, long length,
			    int peakgroup, float* peakX, float* peakY, long j,
			    int n);

static inline int sign(float x)
{
	if (x > 0){
		return 1;
	} else if (x == 0) {
		return 0;
	} else {
		return -1;
	}
}

int findpeaks(const float* x, const float* y, long length,
	      float slopeThreshold, float ampThreshold, float smoothwidth,
	      int peakgroup, int smoothtype, int N, bool first, float* peakX,
	      float* peakY, float* firstPeakX)
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
	// number of returned peaks returned by the function. If `first` is
	// true, then the first `N` peaks are returned. Otherwise, the `N`:
	// peaks with the highest amplitude are returned.

	// "peakX", "peakY", and will point at the peaks with the greatest 

	smoothwidth = round(smoothwidth);
	peakgroup = round(peakgroup);
	float* deriv_vals = deriv(y,length);
	float* d=fastsmooth(deriv_vals, length, smoothwidth, smoothtype);
	free(deriv_vals);
	int n = (int)round(peakgroup/2 +1);

	long j, temp;
	float curPeakX,curPeakY;

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

					if ((first)&(peakQ.cur_size == N) ){
						j = length;
					}
					
					break;
				}
			}
		}
	}
	
	temp = j+1;

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
					if ((first)&(peakQ.cur_size == N) ){
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



void findpeaksHelper(const float* x, const float* y, long length, int peakgroup,
		     float* peakX, float* peakY, long j, int n)
{
	// Helper Function
	if (peakgroup<5){
		*peakY = -1.0;
		// At a glance the following for loop seems wrong (especiall in
		// comparison to the matlab code but I think its actually
		// correct)
		for (int k=1;k<=peakgroup;k++){
			long groupindex = (j+(long)k-(long)n);
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
		float* xx = malloc(sizeof(float)*peakgroup);
		float* yy = malloc(sizeof(float)*peakgroup);
		float mean, std;
		float *coef;
		// At a glance the following for loop seems wrong (especiall
		// in comparison to the matlab code), but I think its actually
		// correct)
		for (int k=1;k<=peakgroup;k++){
			long groupindex = (j+(long)k-(long)n);
			if (groupindex < 0) {
				groupindex = 0;
			}
			if (groupindex >= length) {
				groupindex = length-1;
			}
			xx[k-1]=x[groupindex];
			yy[k-1]=(float)log(fabs((double)(y[groupindex])));
		}
		
		// fit parabola to log10 of sub-group with centering and scaling
		coef = quadFit(xx, yy, peakgroup, &mean, &std);
		
		*peakX = -((std*coef[1]/(2.*coef[2]))-mean);
		// check that we are correctly squaring
		*peakY = (float)exp((double)(coef[0] - coef[2] *
					     pow((coef[1]/(2.*coef[2])),2)));
		free(xx);
		free(yy);
		free(coef);
	}
}

float* quadFit(float* x, float* y, long length, float* mean, float *std)
{
	// Fits a Quadratic to data using least square fitting, like matlab's
	// polyfit function
	// https://www.mathworks.com/help/matlab/ref/polyfit.html
	
	// Like polyfit, we center x at zero and scale it to have 1 unit
	// standard deviation
	// xp = (x-mean)/std
	// Here mean is the average x value and std is the stamdard deviation

	float* coef = malloc(sizeof(float)*3);
	long i;
	*mean=0.0;
	for (i=0;i<length;i++){
		*mean+=x[i];
	}
	*mean/=(float)length;

	float temp;
	// assuming that all values of x are real
	*std=0.0;
	for (i=0;i<length;i++){
		temp=(x[i]-*mean);
		*std+=temp*temp;
	}
	*std = sqrtf(*std/((float)(length-1)));

	// now determine xp

	float* xp = malloc(sizeof(float)*length);
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
	float sxx, sxx2, sx2x2, sxy, sx2y, sumx, sumx2, sumy, cur_x, cur_x2;
	float cur_y;
	sumx = 0;
	sumx2 = 0;
	sumy = 0;
	sxx2 = 0;
	sx2x2 = 0;
        sxy = 0;
	sx2y = 0;

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

	sxx = sumx2 - ((sumx*sumx)/((float)length));
	sxx2 -= ((sumx*sumx2)/((float)length));
	sx2x2 -= ((sumx2*sumx2)/((float)length));
	sxy -= ((sumx*sumy)/((float)length));
	sx2y -= ((sumx2*sumy)/((float)length));

	coef[2] = ((sx2y * sxx)-(sxy * sxx2))/((sxx * sx2x2)-(sxx2 * sxx2));
	coef[1] = ((sxy * sx2x2)-(sx2y * sxx2))/((sxx * sx2x2)-(sxx2 * sxx2));
	coef[0] = (sumy - (coef[1]*sumx)-(coef[2]*sumx2))/((float)length);

	free(xp);
	return coef;
}


float* deriv(const float* a, size_t length)
{
	// First derivative of vector using 2-point central difference.
	// Transcribed from matlab code of T. C. O'Haver, 1988.
	float* d = malloc( sizeof(float) * length);
	d[0] = a[1]-a[0];
	d[length-1] = a[length-1]-a[length-2];
	for(size_t i=1;i<length-1;i++){
		d[i] = (a[i+1]-a[i-1])/2.;
	}
	return d;
}

// this can be optimized significantly
float* fastsmooth(float* y, long length, float w, int type)
{
	// Transcribed from matlab code of T. C. O'Haver, 1988 Version 2.0, 
	// May 2008.
	// smooths array with smooth of width w.
	//  If type=1, rectangular (sliding-average or boxcar)
	//  If type=2, triangular (2 passes of sliding-average)
	//  If type=3, pseudo-Gaussian (3 passes of sliding-average)
	if (type<1 || type > 3){
		printf("The type argument was passed an invalid value\n");
		fflush(stdout);
		abort();
	}

	float *cur = NULL;
	for (int i = 0; i < type; i++){
		float *in = (i == 0) ? y : cur;
		cur = sa(in,length,w);
		if (in != y){
			free(in);
		}
		if (cur == NULL){
			return NULL;
		}
	}
	return cur;
}

float* sa(const float* in, long length, float smoothwidth)
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
	if(smoothwidth < 1.0f){
		smoothwidth = 1.0f;
	}
	
	long w = (long)round(smoothwidth);
	float sumPoints = 0;
	for(long k = 0; k < w; k++){
		sumPoints += in[k];
	}

	// create s which is an array of zeros
	float* s = malloc(sizeof(float)*length);
	for(long k = 0; k < length; k++){
		s[k]=0;
	}

	long halfw = (long)round(((float)w)/2.);
	for(long k = 0; k < length - w; k++){
		s[k+halfw-1]=sumPoints/((float)w);
		sumPoints = sumPoints-in[k]+in[k+w];
	}

	long k = length - w - 1 + halfw;
	for (long i=(length-w);i<length;i++){
		s[k] += in[i];
	}
	s[k]/=((float)w);
	return s;
}

// Implementing a minimum priority queue based on a peak's amplitude (value of
// peakY)

struct peakQueue peakQueueNew(int max_size)
{
	// constructor for peakQueue
	struct peakQueue peakQ;
	peakQ.array = malloc(max_size*sizeof(struct peak));
	peakQ.max_size = max_size;
	peakQ.cur_size = 0;
	return peakQ;
}

void peakQueueDestroy(struct peakQueue peakQ)
{
	// destructor for peakQueue
	free(peakQ.array);
}


void peakQueueSwapPeaks(struct peakQueue *peakQ, int index1, int index2)
{
	// helper function that swaps the peak at index1 with the peak at index
	// 2
	struct peak tempPeak;
	tempPeak = peakQ->array[index1];
	peakQ->array[index1] = peakQ->array[index2];
	peakQ->array[index2]=tempPeak;
}

void peakQueueBubbleUp(struct peakQueue *peakQ,int index)
{	
	// this helper function is called after a new peak has been inserted
	// this restores the heap properties
	// compare the peak at index with its parent
	// the parent has (index-1)/2
	int parentIndex = (index-1)/2;
	
	//check to see if the 
	if (peakQ->array[parentIndex].peakY > peakQ->array[index].peakY){
		peakQueueSwapPeaks(peakQ, index, parentIndex);
		if (parentIndex>0){
			peakQueueBubbleUp(peakQ,parentIndex);
		}
	}
}

void peakQueueBubbleDown(struct peakQueue *peakQ, int index)
{
	// this helper function is called after the minimum has been 
	// replaced with the peak at the last level. This restores 
	// the heap properties

	// Basically we compare the peak at index with both of its 
	// children nodes. If there is a child small than the current 
	// peak, we swap current peak with the child. If both children 
	// are smaller than the current peak, we swap the current 
	// peak with the smaller child.

	// calculate the indices of the children

	int leftIndex = 2*index+1;
	int rightIndex = 2*index+2;

	if (leftIndex >= peakQ->cur_size){
		// in this scenario, the current node has no children
		return;
	} else if (rightIndex>= peakQ->cur_size){
		// in this scenario, the current node only has 1 child
		if (peakQ->array[index].peakY > peakQ->array[leftIndex].peakY){
			peakQueueSwapPeaks(peakQ, index, leftIndex);
		}
		return;
	} else if (peakQ->array[index].peakY > peakQ->array[leftIndex].peakY) {
		// in this scenario, the current node has a smaller peak than 
		// the left child
		if (peakQ->array[leftIndex].peakY>peakQ->array[rightIndex].peakY) {
			// this means the right child is even smaller than the
			// left child
			peakQueueSwapPeaks(peakQ, index, rightIndex);
			peakQueueBubbleDown(peakQ, rightIndex);
		} else {
			// the parent is larger than the left child but not the
			// right child
			peakQueueSwapPeaks(peakQ, index, leftIndex);
			peakQueueBubbleDown(peakQ, leftIndex);
		}
		return;
	} else if (peakQ->array[index].peakY > peakQ->array[rightIndex].peakY) {
		// the parent is larger than the right child but not the left
		// child
		peakQueueSwapPeaks(peakQ, index, rightIndex);
		peakQueueBubbleDown(peakQ, rightIndex);
	} else {
		// the parent is smaller than both the left and the right child
		return;
	}	
}

void peakQueueInsert(struct peakQueue *peakQ, struct peak newPeak)
{
	// insert a new peak into the peak queue
	peakQ->array[peakQ->cur_size]= newPeak;
	if (peakQ->cur_size != 0){
		peakQueueBubbleUp(peakQ,peakQ->cur_size-1);
	}
	peakQ->cur_size+=1;
}

struct peak peakQueuePop(struct peakQueue *peakQ)
{
	// pop the minimum peak off of the peakQ
	// first check to see if there is only one value
	if (peakQ->cur_size == 1){
		peakQ->cur_size = 0;
		return peakQ->array[0];
	} else {
		// swap the minimum value with the last value
		peakQueueSwapPeaks(peakQ, 0, peakQ->cur_size-1);
		peakQ->cur_size-=1;
		peakQueueBubbleDown(peakQ, 0);
		return peakQ->array[peakQ->cur_size];
	}
}

void peakQueueAddNewPeak(struct peakQueue *peakQ, float peakX, 
			 float peakY)
{
	// mutator method to the peakQ
	// This function is in charge of adding new peak data to the peakQ
	// Basically it keeps adding data until the peakQ is full and it 
	// ensures that the peakQ always contains the largest peaks
	
	if (peakQ->cur_size != peakQ->max_size) {
		// this means that we simply insert another peak into the peakQ
		struct peak newPeak;
		newPeak.peakX = peakX;
		newPeak.peakY = peakY;
		peakQueueInsert(peakQ, newPeak);
	} else {
		// the peakQ is full
		// check the value of peakY of the minimum peak. If it's
		// smaller than the value of peakY for the new peak, we will
		// effectively insert the new peak and pop the minimum peak.
		
		if (peakY > peakQ->array[0].peakY) {
			// We cut out the intermediary steps of inserting then
			// popping, we will just by just updating the
			// information of the minimum peak to reflect the data
			// of the new peak and then we will call
			// peakQueueBubbleDown.
			peakQ->array[0].peakX = peakX;
			peakQ->array[0].peakY = peakY;
			peakQueueBubbleDown(peakQ, 0);
		}
	}
}

int peakQueueToArrays(struct peakQueue *peakQ, float* peakX, float* peakY)
{
	// this removes the data from array struct format and places it in
	// arrays this function returns the number of entries in the arrays
	int i;
	int length = peakQ->cur_size;
	struct peak tempPeak;
	for (i=0;i<length;i++){
		tempPeak = peakQueuePop(peakQ);
		peakX[i] = tempPeak.peakX;
		peakY[i] = tempPeak.peakY;
	}
	return length;
}


void peakQueuePrint(struct peakQueue *peakQ)
{
	// print out the peaks in the Queue
	int i,length;
	
	length = peakQ->cur_size;
	struct peak *temp = malloc(length * sizeof(struct peak));
	printf("[");
	
	if (peakQ->cur_size !=0) {
		temp[0] = peakQueuePop(peakQ);
		printf("frequency = %.0f magnitude = %.0f", temp[0].peakX,
		       temp[0].peakY);
	}
	for (i=1;i<length;i++){
		temp[i] = peakQueuePop(peakQ);
		printf(",\n frequency = %.0f magnitude = %.0f", temp[i].peakX,
		       temp[i].peakY);
	}

	// now put everything back into the Queue
	for (i=0;i<length;i++){
		peakQueueInsert(peakQ, temp[i]);
	}

	printf("]\n");
	free(temp);

}
