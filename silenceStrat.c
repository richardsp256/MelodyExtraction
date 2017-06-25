#include <string.h>
#include <ctype.h>
#include "fVADsd.h"
#include "silenceStrat.h"


SilenceStrategyFunc chooseSilenceStrategy(char* name){
	// this function returns the fundamental detection strategy named name
	// all names are case insensitive
	// this function returns NULL if the name is invalid

	SilenceStrategyFunc detectionStrategy;

	// Lets make name lowercased
	// https://stackoverflow.com/questions/2661766/c-convert-a-mixed-case-
	//string-to-all-lower-case

	for(int i = 0; name[i]; i++){
		name[i] = tolower(name[i]);
	}
	
	if (strcmp(name,"fvad")==0) {
		detectionStrategy = &fVADDetectionStrategy;
	} else {
		detectionStrategy = NULL;
	}
	return detectionStrategy;
}

int fVADDetectionStrategy(float** AudioData, int length, int frameLength,
			  int spacing, int samplerate, int mode,
			  int* activityRanges){
	return fVADSilenceDetection(AudioData, samplerate, mode, frameLength,
				    spacing, length, activityRanges);
}
