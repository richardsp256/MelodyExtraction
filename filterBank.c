#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "filterBank.h"

struct filterBank* filterBankNew(int numChannels, int lenChannels, int overlap,
				 int samplerate, float minFreq, float maxFreq){
	struct filterBank* fB;
	float *fcArray;
	int i;
	
	if (numChannels<1){
		printf("Error: numChannels must be postive\n");
		return NULL;
	}
	if (lenChannels<1){
		printf("Error: lenChannels must be postive\n");
		return NULL;
	}
	if (overlap<0){
		printf("Error: overlap must be >=0\n");
		return NULL;
	}
	if (samplerate<1){
		printf("Error: samplerate must be postive\n");
		return NULL;
	}
	if (minFreq<=0){
		printf("Error: minFreq must be >= 0\n");
		return NULL;
	}
	if (maxFreq<minFreq){
		printf("Error: maxFreq must be >= minFreq\n");
		return NULL;
	}
	if ((maxFreq==minFreq) && (numChannels!=1)){
		printf("Error: if maxFreq == minFreq, numChannels must be 1\n");
		return NULL;
	}
	
	fB = malloc(sizeof(struct filterBank));
	if (fB == NULL){
		return fB;
	}
	(fB->cDArray) = malloc(sizeof(struct channelData)*numChannels);

	if ((fB->cDArray) == NULL){
		free(fB);
		return fB;
	}

	fcArray = centralFreqMapper(numChannels, minFreq, maxFreq);
	if (fcArray == NULL){
		filterBankDestroy(fB);
		return NULL;
	}
	
	for (i=0; i<numChannels; i++){
		// going to need to be modified
		((fB->cDArray)[i]).cf = fcArray[i];

		*((fB->cDArray)[i]).p1r = 0.0;
		*((fB->cDArray)[i]).p2r = 0.0;
		*((fB->cDArray)[i]).p3r = 0.0;
		*((fB->cDArray)[i]).p4r = 0.0;
		*((fB->cDArray)[i]).p1i = 0.0;
		*((fB->cDArray)[i]).p2i = 0.0;
		*((fB->cDArray)[i]).p3i = 0.0;
		*((fB->cDArray)[i]).p4i = 0.0;
		*((fB->cDArray)[i]).qcos = 1.0;
		*((fB->cDArray)[i]).qsin = 0.0;

	}
	fB->numChannels = numChannels;
	fB->lenChannels = fB->lenChannels;
	fB->overlap = overlap;
	fB->samplerate = samplerate;

	free(fcArray);
	
	return fB;
}

void filterBankDestroy(struct filterBank* fB){
	free(fB->cDArray);
	free(fB);
}


float* centralFreqMapper(int numChannels, float minFreq, float maxFreq){
	/* The paper states that the frequencies are mapped according to the 
	 * according to the Equivalent Rectangular Bandwifth (ERB) scale.
	 * According to wikipedia, 
	 * https://en.wikipedia.org/wiki/Equivalent_rectangular_bandwidth,
	 * a linear approximation of ERB between 100 and 10000 Hz, is given by
	 * ERB = 24.7 * (4.37 * f + 1), where ERB is in units of Hz, and f is 
	 * in units in kHz. We can rewrite this equation where f is in Hz as:
	 * ERB = 24.7 * (0.00437 * f + 1).
	 * We will use this linear approximation since it is the same as the 
	 * one used by our gammatone filter.
	 *
	 * According to the same page, the ERB scale (ERBS) for this linear 
	 * approximation is given by:
	 * ERBS = 21.3 * log10(1+0.00437*f) where f has units of Hz.
	 * We calculate the inverse to be:
	 * f = (10^(ERBS/21.4) - 1)/0.00437
	 *
	 * Onto the problem at hand. we will return an array of central 
	 * frequencies, fcArray, of length = numChannels.
	 *
	 * If numChannels == 1, then the only entry will be minFreq
	 *
	 * In general, the minimum and maximum 
	 * frequencies (fmin and fmax) included in a bandwidth B, centred on 
	 * fc are given by: fmin = fc - B/2 and fmax = fc + B/2
	 * In function calls problems where there are at least 2 channels, 
	 * we will choose the very first and the very last central frequencies 
	 * such that the minFreq and maxFreq are included in the edge of the 
	 * bandwidths. In other words: 
	 * minFreq = min(fcArray) - B/2 and maxFreq = max(fcArray) + B/2
	 * plugging in ERB = 24.7 * (0.00437 * fc + 1) for B in each equation 
	 * we can solve for min(fcArray) and max(fcArray):
	 * min(fcArray) = (minFreq + 12.35)/0.9460305
	 * max(fcArray) = (maxFreq - 12.35)/1.0539695
	 *
	 * then I will space out the remaining central frequencies with 
	 * evenly with respect to ERBS
	 * Let minERBS = ERBS(min(fcArray)) and maxERBS = ERBS(max(fcArray))
	 * The ith entry of fcArray will have ERBS given by
	 * ERBS(fcArray[i]) = minERBS + i * (maxERBS-minERBS)/(numChannels - 1)
	 *
	 * We can calculate the ith entry of fcArray by taking the inverse of 
	 * the ERBS of the ith entry
	 * fcArray[i] =(10.^((minERBS + i * (maxERBS-minERBS)/(numChannels - 1))
	 *                   /21.4) - 1)/0.00437
	 */
	float *fcArray, minERBS,maxERBS;
	int i;
	
	fcArray = malloc(sizeof(float)*numChannels);
	if (fcArray ==NULL){
		return NULL;
	}

	if (numChannels == 1){
		fcArray[0] = minFreq;
		return fcArray;
	}
	
	/* calculate the minimum central frequency */
	fcArray[0] = (minFreq + 12.35)/0.9460305;

	/* calculate the maximum central frequency */
	fcArray[numChannels-1] = (maxFreq - 12.35)/1.0539695;

	/* calculate minERBS and maxERBS */
	minERBS = 21.3 * log10f(1+0.00437*fcArray[0]);
	maxERBS = 21.3 * log10f(1+0.00437*fcArray[numChannels-1]);

	/* calculate all other entries of fcArray */
	for (i=1;i<numChannels-1;i++){
		fcArray[i] = ((powf(10.,((minERBS + (float)i * (maxERBS-minERBS)
					  /((float)numChannels - 1))/21.4))
			       - 1.0) /0.00437);
	}
	return fcArray;
}

void swap_chunk(float **a, float **b){
	float *temp= *a;
	*a = *b;
	*b = temp;
}

void filterBankFilteringHelper(struct channelData * cD,
			       float* inputChunk,
			       float** leadingSpectraChunk,
			       int nsamples, int samplerate){
	gammatoneFilterChunk(inputChunk, leadingSpectraChunk, cD->cf,
			     samplerate, nsamples, (cD->p1r), (cD->p2r),
			     (cD->p3r), (cD->p4r), (cD->p1i), (cD->p2i),
			     (cD->p3i), (cD->p4i), (cD->qcos), (cD->qsin));
}

void filterBankOverlapHelper(float** leadingSpectraChunk,
			     float** trailingSpectraChunk,
			     int nsamples){
	int i;
	for (i=0;i<nsamples;i++){
		(*leadingSpectraChunk)[i] = (*trailingSpectraChunk)[i];
	}
}

void filterBankFirstChunk(struct filterBank* fB, float* inputChunk,
			  int nsamples, float** leadingSpectraChunk){
	int i, offset;
	float* cur_chunk;
	
	for (i=0;i<(fB->numChannels);i++){
		offset = i*(fB->lenChannels);
		cur_chunk = (*leadingSpectraChunk)+offset;
		filterBankFilteringHelper(((fB->cDArray)+i),
					  inputChunk,
					  (&cur_chunk),
					  nsamples, (fB->samplerate));
	}
}

void filterBankUpdateChunk(struct filterBank* fB, float* inputChunk,
			   int nsamples, float** leadingSpectraChunk,
			   float** trailingSpectraChunk){
	int i, offset, overlap;
	float *curLeadingRow, *curTrailingRow;
	/* first we swap the spectra chunks */
	swap_chunk(leadingSpectraChunk, trailingSpectraChunk);

	/* now we iterate over the channels in leading SpectraChunk and
	 * overwrite them with data corresponding to the inputChunk.
	 * For each channel:
	 * - we need to copy the overlapping data at the end of 
	 *   trailingSpectraChunk into the start of leadingSpectraChunk
	 * - we need to apply gammatone filtering on input chunk and fill in 
	 *   the rest of the channel buffer 
	 */

	overlap = fB->overlap;
	
	for (i=0;i<(fB->numChannels);i++){
		offset = i*(fB->lenChannels);

		curLeadingRow = (*leadingSpectraChunk)+offset;
		curTrailingRow = (*trailingSpectraChunk)+offset;
		
		/* first we copy in the overlapping data */
		//filterBankOverlapHelper(&((*leadingSpectraChunk)+offset),
		//			&((*trailingSpectraChunk)+offset),
		//			overlap);
		filterBankOverlapHelper(&curLeadingRow,
					&curTrailingRow,
					overlap);
	

		/* now we apply the filter to inputChunk and fill in the rest 
		 * of the channel.
		 */
		offset += overlap;
		curLeadingRow = (*leadingSpectraChunk)+offset;
		//filterBankFilteringHelper(&((fB->cDArray)[i]),
		//			  inputChunk,
		//			  &((*leadingSpectraChunk)+offset),
		//			  nsamples, (fB->samplerate));
		filterBankFilteringHelper(((fB->cDArray)+i),
					  inputChunk,
					  &curLeadingRow,
					  nsamples, (fB->samplerate));
	}
}


void filterBankFinalChunk(struct filterBank* fB, float* inputChunk,
			  int nsamples, float** leadingSpectraChunk,
			  float** trailingSpectraChunk){
	int i,j,offset, overlap,lenChannels;
	float *curLeadingRow, *curTrailingRow;
	/* first we swap the spectra chunks */
	swap_chunk(leadingSpectraChunk, trailingSpectraChunk);

	/* now we iterate over the channels in leading SpectraChunk and
	 * overwrite them with data corresponding to the inputChunk.
	 * For each channel:
	 * - we need to copy the overlapping data at the end of 
	 *   trailingSpectraChunk into the start of leadingSpectraChunk
	 * - we need to apply gammatone filtering on input chunk and fill in 
	 *   the channel buffer 
	 * - fill in the rest of the channel buffer with 0s
	 */

	overlap = fB->overlap;
	lenChannels = fB->lenChannels;
	
	for (i=0;i<(fB->numChannels);i++){
		offset = i*lenChannels;

		curLeadingRow = (*leadingSpectraChunk)+offset;
		curTrailingRow = (*trailingSpectraChunk)+offset;
		
		/* first we copy in the overlapping data */
		//filterBankOverlapHelper(&((*leadingSpectraChunk)+offset),
		//			&((*trailingSpectraChunk)+offset),
		//			overlap);
		filterBankOverlapHelper(&curLeadingRow, &curTrailingRow,
					overlap);

		/* now we apply the filter to inputChunk and fill in the rest 
		 * of the channel.
		 */
		offset += overlap;
		curLeadingRow = (*leadingSpectraChunk)+offset;
		if (nsamples > 0){
			filterBankFilteringHelper(((fB->cDArray)+i),
						  inputChunk,
						  &curLeadingRow,
						  nsamples, (fB->samplerate));
			/* finally we fill in the rest of the channels */
			offset += nsamples;
		}

		/* pad the remainder with zeros */
		for (j=offset; (j<((i+1)*lenChannels));j++){
			(*leadingSpectraChunk)[j] = 0.0;
		}
	}
}









/*


void printArray(int *array, int length){
	printf("[");
	if (length >0){
		printf("%d", array[0]);
	}
	
	for (int i=1;i<length;i++){
		printf(", %d",array[i]);
	}
	printf("]\n");

}

int main(int argc, char ** argv)
{
	int* array;

	array = malloc(sizeof(int) * 6);
	array[0] = 0;
	array[1] = 1;
	array[2] = 2;
	array[3] = 3;
	array[4] = 4;
	array[5] = 5;

	printArray(array,6);
	printArray(array+3,3);
	free(array);
	return 0;
}
*/
