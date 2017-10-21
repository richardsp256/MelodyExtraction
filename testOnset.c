#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <float.h>

//wmin set to 4 (20ms)
//wmax set to 500 (2.5s)
//based on correntropy hopsize h being 5ms

//Aw = Kernel of len w
//Ww = window of detection function W of len w
//Fitness(Aw, Ww) = (Aw - Ww)^2 * w^(-1)

//Aw = z/(1 + a - |z|), a = 0.15, z is split into w evenly spaced values on range  -1+10^-5 <= z <= 1-10^-5
float MINZ = -0.99999f;
float MAXZ = 0.99999f;
float DOUBLEMAXZ = 1.99998f;

float calcZ(int index, int max)
{
	return ((DOUBLEMAXZ * index) / (max-1)) + MINZ;
}

float** GenKernels(int minK, int maxK)
{
	int numKernels = maxK - minK + 1;
	int i, j, kernelLen;
	float z;
	if(numKernels <= 0){
		return NULL;
	}
	float** kernels = malloc(numKernels * sizeof(float*));
	for(i = 0, kernelLen = minK; i < numKernels; i++, kernelLen++)
	{
		kernels[i] = malloc(kernelLen * sizeof(float));
		for(j = 0; j < kernelLen; ++j){
			z = calcZ(j, kernelLen);
			kernels[i][j] = z/(1.15f - fabs(z));
		}
	}
	return kernels;
}

void freeKernels(float** kernels, int numKernels){
	for(int i = 0; i < numKernels; ++i){
		free(kernels[i]);
	}
	free(kernels);
}

float FitnessOnset(float* kernel, float* window, int start, int len)
{
	float sum = 0.0f;
	float* winptr = &window[start];
	for(int i = 0; i < len; ++i, ++winptr){
		sum += (kernel[i] - (*winptr))*(kernel[i] - (*winptr));
	}
	return sum/len;
}

float FitnessOffset(float* kernel, float* window, int start, int len)
{
	//only difference from FitnessOnset is kernel[i] is negated
	float sum = 0.0f;
	float* winptr = &window[start];
	for(int i = 0; i < len; ++i, ++winptr){
		sum += (-kernel[i] - (*winptr))*(-kernel[i] - (*winptr));
	}
	return sum/len;
}

//adds [value] to [transients] at index [index]. if the index is out of bounds, resize the array.
void AddTransientAt(int** transients, int* size, int value, int index ){
	if(index >= (*size)){ //out of space in transients array
		(*size) *= 2;
		int* temp = realloc((*transients),(*size)*sizeof(int));
		if(temp != NULL){
			(*transients) = temp;
		} else { //realloc failed
			free(*transients);
			(*size) = -1;
			return;
		}
	}
	(*transients)[index] = value;
}

//populates transients, which will hold indices of onsets and offsets in detection_func
int detectTransients(int** transients, float* detection_func, int len){
	int minkernel = 4;
	int maxkernel = 1500;

	printf("detection with len %d, minK %d, maxK %d\n", len, minkernel, maxkernel);

	float** Kernels = GenKernels(minkernel, maxkernel);
	int numKernels = maxkernel - minkernel + 1;

	printf("kernel made\n");

	int transients_capacity = 20; //initial size for onsets array. we will realloc for more space as needed
	int transient_index = 0;
	int detect_index = 0;
	int tmpMax = 0;
	int lastpossibleStart = len-8; //last possible starting index bc notes are at least 8 indices for onset+offset.
	int lastpossibleOnset = len-4; //last possible onset index bc offset requires at least 4 indices.

	float bestFitness;
	int bestInd;

	float curFitness;
	int i;

	(*transients) = malloc(transients_capacity * sizeof(int));

	while(detect_index < lastpossibleStart){

		//printf("start while index - %d\n", detect_index);

		bestFitness = -FLT_MAX;
		bestInd = 0;
		tmpMax = maxkernel < (len - detect_index) ? maxkernel : (len - detect_index);
		for(i = minkernel; i < tmpMax; ++i){ 
			//printf("test onset %d of %d\n", i, tmpMax);
			curFitness = FitnessOnset(Kernels[i - minkernel], detection_func, detect_index, i);
			if(curFitness > bestFitness){
				bestFitness = curFitness;
				bestInd = i;
			}
		}
		detect_index += bestInd;
		if(detect_index >= lastpossibleOnset){
			//onset detected too close to end for a corresponding offset, so we skip the onset and end.
			//printf("onset detected too close to end, break\n");
			break;
		}

		AddTransientAt(transients, &transients_capacity, detect_index, transient_index);
		if(transients_capacity == -1){ //resize failed
			printf("Resizing transients failed. Exitting.\n");
			freeKernels(Kernels, numKernels);
			free(*transients);
			return -1;
		}
		transient_index += 1;

		bestFitness = -FLT_MAX;
		bestInd = 0;
		tmpMax = maxkernel < (len - detect_index) ? maxkernel : (len - detect_index);
		for(i = minkernel; i < tmpMax; ++i){
			curFitness = FitnessOffset(Kernels[i - minkernel], detection_func, detect_index, i);
			if(curFitness > bestFitness){
				bestFitness = curFitness;
				bestInd = i;
			}
		}
		detect_index += bestInd;

		AddTransientAt(transients, &transients_capacity, detect_index, transient_index);
		if(transients_capacity == -1){ //resize failed
			printf("Resizing transients failed. Exitting.\n");
			freeKernels(Kernels, numKernels);
			free(*transients);
			return -1;
		}
		transient_index += 1;
		
		//if at end of activity range, jump detect_index forward to start of next range
	}

	freeKernels(Kernels, numKernels);

	if(transient_index == 0){ //if no transients found, do not realloc here. Unable to realloc array to size 0
		return transient_index;
	}

	int* temp = realloc((*transients),transient_index*sizeof(int));
	if (temp != NULL){
		(*transients) = temp;
	} else {
		printf("Resizing onsets failed. Exitting.\n");
		free(*transients);
		return -1;
	}

	return transient_index;
}