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
void fractPartArray(float* input, int size, float** output)
{
	(*output) = malloc(sizeof(float) * size);
	for(int i = 0; i < size; i++)
	{
		(*output)[i] = fractPart(input[i]);
	}
}

float fractPart(float x)
{
	return x - (int)x;
}

void weightNoteDist(float* input, int size, float center, float** weights)
{
	(*weights) = malloc(sizeof(float) * size);
	for(int i = 0; i < size; i++)
	{
		(*weights)[i] = 1.0f/(input[i] - center + 1); //dist 0 = 1, dist 1 = 1/2, dist 2 = 1/3, dist 3 = 1/4, ect.
	}
}

float FractionalAverage(float* arr, int len, int centerInd, float* avg)
{
	assert(centerInd < len && centerInd >= 0);

	float* fractParts;
	fractPartArray(arr, len, &fractParts);

	float* weights;
	float center = arr[centerInd];
	printf("center  %f\n", center);
	weightNoteDist(arr, len, center, &weights);

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

float linearDist(float* arr, int len, float pt)
{
	//calc linear dist of all nums in arr to pt.
	//aka  sum(abs(arr[i] - pt))
	//note: for median, as median is the pt with the minimum linear dist
	float dist = 0;
	for(int i = 0; i < len; ++i){
		dist = dist + fabs(arr[i] - pt);
	}
	return dist;
}
float squareDistWrapped(float* arr, int len, float pt)
{
	//calc square dist of all nums in arr to pt.
	//aka  sum(sqr(arr[i] - pt))
	//note: for mean, as mean is the pt with the minimum square dist
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
	return dist;
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

float mean(float* arr, int len)
{
	float avg = 0;
	for(int i = 0; i < len; i++){
		avg = avg + arr[i];
	}
	avg = avg / len;
	return avg;
}

float sum(float* arr, int len)
{
	float sum = 0.0f;
	for(int i = 0; i < len; ++i){
		sum = sum + arr[i];
	}
	return sum;
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

float squareDistWrappedWeighted(float* arr, int len, float pt, float* weights)
{
	//calc square dist of all nums in arr to pt.
	//aka  sum(sqr(arr[i] - pt))
	//note: for mean, as mean is the pt with the minimum square dist
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
	return dist;
}