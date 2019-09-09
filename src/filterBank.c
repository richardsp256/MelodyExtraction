#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "filterBank.h"

struct filterBank* filterBankNew(int numChannels, int lenChannels, int overlap,
				 int samplerate, float minFreq, float maxFreq)
{
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

	//printf("Constructing the channel Data array\n");
	fcArray = malloc(sizeof(float)*numChannels);
	if (fcArray == NULL){
		filterBankDestroy(fB);
		return NULL;
	}
	centralFreqMapper(numChannels, minFreq, maxFreq, fcArray);

	/*
	for (i=0; i<numChannels; i++){
		// fill in the channel data

	}
	*/
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


void centralFreqMapper(int numChannels, float minFreq, float maxFreq,
		       float* fcArray){
	float minERBS,maxERBS;
	int i;

	if (numChannels == 1){
		fcArray[0] = minFreq;
		return;
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
	//gammatoneFilterChunk();
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
