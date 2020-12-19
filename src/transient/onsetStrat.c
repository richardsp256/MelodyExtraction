#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include "../resample.h"
#include "../errors.h"
#include "onsetStrat.h"
#include "pairTransientDetection.h"
#include "simpleDetFunc.h"

// Note: This function signature is a little hacky. DftBlocksize is never used
// and onsets contains all transients (including onsets and offsets)
// May want to rename this something like "PairwiseTransientStrategy"
int TransientDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			       int samplerate, intList* onsets)
{
	printf("in transientDetectionStrategy\n");

	//downsample audiodata to 11025 using libsamplerate
	int samplerateOld = samplerate;
	samplerate = 11025;
	float* ResampledAudio = NULL;
	float sampleRatio = samplerate/((float)samplerateOld);
	int RALength = ResampleAndAlloc(*AudioData, size, sampleRatio, &ResampledAudio);
	if(RALength < 0){ // negative values encode errors
		return RALength;
	}

	printf("resample complete\n");

	int transientsLength = 
		pairwiseTransientDetection(ResampledAudio, RALength,
					   samplerate, onsets);

	if(transientsLength <= 0){
		free(ResampledAudio);
		return transientsLength;
	}

	// convert onsets so that the sample corresponds to the old sample rate
	// rather than the resampled audio samplerate
	int* o_arr = onsets->array;
	for(int i = 0; i < transientsLength; ++i){
		o_arr[i] *= (int)((samplerateOld / (float)samplerate));
	}

	for(int k = 0; k < transientsLength; k+=2){
		printf("  %d - %d   (%dms - %dms)\n",
		       o_arr[k], o_arr[k+1],
		       (int)(o_arr[k]*(1000/(float)samplerateOld)),
		       (int)(o_arr[k+1]*(1000/(float)samplerateOld)) );
	}
	printf("done, %d notes found\n", transientsLength / 2);


	free(ResampledAudio);
	return transientsLength;
}
