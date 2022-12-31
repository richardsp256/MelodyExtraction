#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h> // powf
#include <stdbool.h>

#include "../resample.h"
#include "../errors.h"
#include "selectTransients.h" // SelectTransients
#include "calcDetFunc.h"

static int coercePosMultipleOf4_(int arg, bool round_up){
	if ((arg <=0) || (arg % 4) == 0){
		return arg;
	} else if (arg < 4) {
		return 4;
	} else if (round_up) {
		return 4*((arg/4) + 1);
	} else {
		return 4*(arg/4);
	}
}

int DetectTransients(float *audioData, int size, int samplerate,
		     intList* transients){
	// use parameters suggested by paper
	int numChannels = 64;
	float minFreq = 80.f; // 80 Hz
	float maxFreq = 4000.f; // 4000 Hz
	int correntropyWinSize = samplerate/80; // assumes minFreq=80
	if ((samplerate % 80) == 0) {
		correntropyWinSize++;
	}
	int interval = samplerate/200; // 5ms
	float scaleFactor = powf(4./3.,0.2); // magic, grants three wishes
	                                     // (Silverman's rule of thumb)
	int sigWindowSize = (samplerate*7); // 7s

	// implementation requirement for correntropyWinSize & interval:
	correntropyWinSize = coercePosMultipleOf4_(correntropyWinSize, true);
	interval = coercePosMultipleOf4_(interval, false);

	// allocate the detectionFunction
	int detectionFunctionLength =
		computeDetFunctionLength(size, correntropyWinSize, interval);
	float* detectionFunction = malloc(sizeof(float) *
					  detectionFunctionLength);

	// compute the detectionFunction
	int rslt = CalcDetFunc(correntropyWinSize, interval, scaleFactor,
			       sigWindowSize, numChannels, minFreq, maxFreq,
			       samplerate, size, audioData,
			       detectionFunctionLength, detectionFunction);
	if (ME_SUCCESS != rslt){
		free(detectionFunction);
		return rslt;
	}

	printf("detect func computed\n");

	// idendify the transients
	int transientsLength = SelectTransients(detectionFunction,
						detectionFunctionLength,
						transients);

	if(transientsLength == -1){
		printf("detectTransients failed\n");
		free(detectionFunction);
		return -1;
	}

	printf("transients created\n");

	// convert transients so that the values are not the indices of
	// detectionFunction, but corresponds to the frame of audioData
	int* t_arr = transients->array;
	for(int i = 0; i < transientsLength; ++i){
		t_arr[i] = t_arr[i] * interval;
	}

	return transientsLength;
}

int DetectTransientsFromResampled(const float* AudioData, int size,
				  int samplerate, intList* transients)
{
	printf("in transientDetectionStrategy\n");

	//downsample audiodata to 11025 using libsamplerate
	int samplerateOld = samplerate;
	samplerate = 11025;
	float* ResampledAudio = NULL;
	float sampleRatio = samplerate/((float)samplerateOld);
	int RALength = ResampleAndAlloc(AudioData, size, sampleRatio, &ResampledAudio);
	if(RALength < 0){ // negative values encode errors
		return RALength;
	}

	printf("resample complete\n");

	int transientsLength = DetectTransients(ResampledAudio, RALength,
						samplerate, transients);

	if(transientsLength <= 0){
		free(ResampledAudio);
		return transientsLength;
	}

	// convert transients so that the sample corresponds to the old sample
	// rate rather than the resampled audio samplerate
	int* t_arr = transients->array;
	for(int i = 0; i < transientsLength; ++i){
		t_arr[i] *= (int)((samplerateOld / (float)samplerate));
	}

	for(int k = 0; k < transientsLength; k+=2){
		printf("  %d - %d   (%dms - %dms)\n",
		       t_arr[k], t_arr[k+1],
		       (int)(t_arr[k]*(1000/(float)samplerateOld)),
		       (int)(t_arr[k+1]*(1000/(float)samplerateOld)) );
	}
	printf("done, %d notes found\n", transientsLength / 2);


	free(ResampledAudio);
	return transientsLength;
}
