#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "tuningAdjustment.h"

//#include <stdio.h>

//given a list of fractional parts (between 0 and 1)
//return 'average' fractional part.
//note that fractional parts can wrap (aka dist between .9 and .1 is .2, not .8)

float FractionalAverage(float* arr, int len, int centerInd, float* avg)
{
	assert(centerInd < len && centerInd >= 0);
	float center = arr[centerInd];
	//printf("center  %f\n", center);

	//printf("array:  ");
	//for(int i = 0; i < len; ++i){
	//	printf("%f ", arr[i]);
	//}
	//printf("\n");

	float* fractParts = malloc(sizeof(float) *  len);
	for(int i = 0; i < len; ++i){
		fractParts[i] = fractPart(arr[i]);
	}

	//printf("weights: ");
	float* weights = malloc(sizeof(float) * len);
	for(int i = 0; i < len; ++i){
		weights[i] = weightDist(arr[i], center);
	//	printf("%f ", weights[i]);
	}
	//printf("\n");

	(*avg) = fractParts[centerInd]; //set initial avg on central pt
	float lowest_dist;
	float mi;

	//start by looking at when midpoint is 0. 
	//so all values .5 and higher are subtracted by 1. 
	//.5 is included bc 0.5 would round to 1, -.5 correctly rounds to 0
	for(int i = 0; i < len; ++i){
		if(fractParts[i] >= 0.5){
			fractParts[i] = fractParts[i] - 1;
		}
	}

	mi = -1000; //min float
	lowest_dist = 100000; //max float
	do{
		//add 1 to all values equal to min (none first time around)
		for(int i = 0; i < len; ++i){
			if(fractParts[i] <= mi){
				fractParts[i] += 1;
			}
		}
		//calc avg and dist (median and linear dist, or mean and square dist)
		//float curAvg = mean(fractParts, len);
		//float curDist = squareDistWrapped(fractParts, len, curAvg);
		float curAvg = meanWeighted(fractParts, len, weights);
		float curDist = squareDistWrappedWeighted(fractParts, len, curAvg, weights);
		//if beats best, update best
		if(curDist < lowest_dist){
			(*avg) = curAvg;
			lowest_dist = curDist;
		}

		mi = min(fractParts, len);
	} while (mi < 0.5);

	return lowest_dist;
}

float fractPart(float x)
{
	return x - (int)x;
}

float weightDist(float a, float b)
{
	if(sqrtf(fabs(a-b)) == 0.0f){ //special case to avoid divide by 0
		return 1.0f;
	}
	return fminf(1.0f/sqrtf(fabs(a - b)), 1.0f);
}

float squareDistWrapped(float* arr, int len, float pt)
{
	//calc square dist of all nums in arr to pt.
	//aka  sum(sq(arr[i] - pt))
	//note: for mean, as mean is the pt with the minimum square dist
	//normalized so dist should always range between 0 and .25
	float dist = 0;
	for(int i = 0; i < len; ++i){
		float diff = arr[i] - pt;
		if(diff > 0.5){
			diff = 1 - diff;
		}
		else if(diff < (-0.5)){
			diff = -1 - diff;
		}
		dist = dist + (diff*diff);
	}
	dist = dist / len;
	return dist;
}

float squareDistWrappedWeighted(float* arr, int len, float pt, float* weights)
{
	//calc weighted square dist of all nums in arr to pt.
	//aka  sum(sqr(arr[i] - pt)*weights[i])
	//note: for mean, as mean is the pt with the minimum square dist
	//normalized so dist should always range between 0 and .25
	float dist = 0;
	for(int i = 0; i < len; ++i){
		float diff = arr[i] - pt;
		if(diff > 0.5){
			diff = 1 - diff;
		}
		else if(diff < (-0.5)){
			diff = -1 - diff;
		}
		dist = dist + (diff*diff * weights[i]);
	}
	dist = dist / (sum(weights, len) - 1);
	return dist;
}

float mean(float* arr, int len)
{
	float avg = 0;
	for(int i = 0; i < len; i++){
		avg = avg + arr[i];
	}
	avg = avg / len;
	return avg;
}

float meanWeighted(float* arr, int len, float* weights)
{
	float avg = 0;
	for(int i = 0; i < len; i++){
		avg = avg + (arr[i] * weights[i]);
	}
	avg = avg / sum(weights, len);
	return avg;
}

float min(float* arr, int len)
{
	if(len == 0){
		return INT_MAX; //max float
	}
	float min = arr[0];
	for(int i = 1; i < len; ++i){
		min = fminf(min, arr[i]);
	}
	return min;
}

float sum(float* arr, int len)
{
	float sum = 0.0f;
	for(int i = 0; i < len; ++i){
		sum = sum + arr[i];
	}
	return sum;
}