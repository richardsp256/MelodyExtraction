#include "melodyextraction.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

#include "extractMelodyProcedure.h"
#include "pitch/pitchStrat.h"
#include "silenceStrat.h"


//default arg settings:
//for pitch and default padding = windowsize, default spacing = windowsize/2
//for silence, default spacing = windowsize
int PITCH_WINDOW_DEF = 4096;
PitchStrategyFunc PITCH_STRATEGY_DEF = &BaNaMusicDetectionStrategy;

int SILENCE_WINDOW_DEF = 10; //silence window is in ms, not num samples
int SILENCE_MODE_DEF = 0;
SilenceStrategyFunc SILENCE_STRATEGY_DEF = &fVADDetectionStrategy;

int msToFrames(int ms, int samplerate){
	return (samplerate * ms) / 1000; //integer division
}

int numParser(char* buf, int* num){
	// returns 1 if buffer is in units of ms. Otherwise returns 0
	char* endptr;
	*num = 0;
	long longnum = strtol(buf, &endptr, 10);
	if (endptr == buf){
		return 0;
	}
	if (longnum > INT_MAX || longnum < INT_MIN){
		return 0;
	}
	*num = longnum;
	if (*endptr!= '\0'){ //characters exist after the parsed number
		if(strcmp(endptr, "ms") == 0){ //proper millisecond format
			return 1;
		}
		else{ //invalid format
			*num = 0;
		}
	}
	return 0;
}


int ConvertToFrames(char* buf, int samplerate){
	
	int num;
	if (numParser(buf, &num) == 1){
		num = msToFrames(num, samplerate);
	}
	return num;
}

struct me_data{
	char * prefix;
	int pitch_window;
	int pitch_padded;
	int pitch_spacing;
	PitchStrategyFunc pitch_strategy;
	int silence_window;
	int silence_spacing;
	SilenceStrategyFunc silence_strategy;
	int silence_mode;
	int hps;
	int tuning;
	int verbose;
};


char* me_data_init(struct me_data** inst, struct me_settings* settings, audioInfo info)
{
	(*inst) = (struct me_data*) calloc(1, sizeof(struct me_data));

	if(settings->prefix != NULL){
		(*inst)->prefix = strdup(settings->prefix);
	}

	if(settings->pitch_window == NULL){
		(*inst)->pitch_window = PITCH_WINDOW_DEF;
	}else{
		(*inst)->pitch_window = ConvertToFrames(settings->pitch_window, info.samplerate);
		if((*inst)->pitch_window < 1){
			me_data_free((*inst));
			(*inst) = NULL;
			return "pitch_window must be a positive int";
		}
	}

	if(settings->pitch_padded == NULL){
		(*inst)->pitch_padded = (*inst)->pitch_window;
	}else{
		(*inst)->pitch_padded = ConvertToFrames(settings->pitch_padded, info.samplerate);
		if((*inst)->pitch_padded < (*inst)->pitch_window ){
			me_data_free((*inst));
			(*inst) = NULL;
			return "pitch_padded cannot be less than pitch_window";
		}
	}

	if(settings->pitch_spacing == NULL){
		(*inst)->pitch_spacing = (int) ceil((*inst)->pitch_window / 2.0f); //ceil to be sure pitch_spacing isnt 0
	}else{
		(*inst)->pitch_spacing = ConvertToFrames(settings->pitch_spacing, info.samplerate);
		if((*inst)->pitch_spacing < 1){
			me_data_free((*inst));
			(*inst) = NULL;
			return "pitch_spacing must be a positive int";
		}
	}

	if(settings->pitch_strategy == NULL){
		(*inst)->pitch_strategy = PITCH_STRATEGY_DEF; //ceil to be sure pitch_spacing isnt 0
	}else{
		(*inst)->pitch_strategy = choosePitchStrategy(settings->pitch_strategy);
		if((*inst)->pitch_strategy == NULL){
			me_data_free((*inst));
			(*inst) = NULL;
			return "pitch_strategy must be \"HPS\", \"BaNa\", or \"BaNaMusic\"";
		}
	}

	if(settings->silence_window == NULL){
		(*inst)->silence_window = SILENCE_WINDOW_DEF;
	}else{
		if (strcmp(settings->silence_window,"10ms") == 0){
			(*inst)->silence_window = 10;
		} else if (strcmp(settings->silence_window,"20ms") == 0){
			(*inst)->silence_window = 20;
		} else if (strcmp(settings->silence_window,"30ms") == 0){
			(*inst)->silence_window = 30;
		} else {
			me_data_free((*inst));
			(*inst) = NULL;
			return "silence_window can only be \"10ms\", \"20ms\", or \"30ms\"";
		}
	}

	if (settings->silence_spacing == NULL){
		(*inst)->silence_spacing = (*inst)->silence_window;
	} else {
		if (numParser(settings->silence_spacing, &((*inst)->silence_spacing))==0) {
			me_data_free((*inst));
			(*inst) = NULL;
			return "silence_spacing must be in units of ms";
		}else if((*inst)->silence_spacing < 1){
			me_data_free((*inst));
			(*inst) = NULL;
			return "silence spacing must be a positive int";
		}
	}

	if(settings->silence_strategy == NULL){
		(*inst)->silence_strategy = SILENCE_STRATEGY_DEF;
	}else{
		(*inst)->silence_strategy = chooseSilenceStrategy(settings->silence_strategy);
		if((*inst)->silence_strategy == NULL){
			me_data_free((*inst));
			(*inst) = NULL;
			return "silence_strategy must be \"fVAD\"";
		}
	}

	(*inst)->silence_mode = settings->silence_mode;
	if((*inst)->silence_mode < 0 || (*inst)->silence_mode > 3){
		me_data_free((*inst));
		(*inst) = NULL;
		return "silence_mode must be 0, 1, 2, or 3";
	}

	(*inst)->hps = settings->hps;
	if((*inst)->hps < 0){
		me_data_free((*inst));
		(*inst) = NULL;
		return "hps must be a positive int";
	}

	(*inst)->verbose = settings->verbose;
	if((*inst)->verbose < 0 || (*inst)->verbose > 1){
		me_data_free((*inst));
		(*inst) = NULL;
		return "verbose must be 0 or 1";
	}

	(*inst)->tuning = settings->tuning;
	if((*inst)->tuning < 0 || (*inst)->tuning > 2){
		me_data_free((*inst));
		(*inst) = NULL;
		return "tuning must be 0, 1, or 2";
	}

	return "";
}

struct me_settings* me_settings_new(){
	struct me_settings *inst = calloc(1, sizeof(struct me_settings));
	inst->silence_mode = 3;
	inst->hps = 2;
	inst->verbose = 0;
	inst->tuning = 1;
	return inst;
}

void me_data_free(struct me_data* inst){
	if(inst->prefix != NULL){
		free(inst->prefix);
	}
	free(inst);
}

void me_settings_free(struct me_settings* inst)
{
	if(inst->prefix != NULL){
		free(inst->prefix);
	}
	if(inst->pitch_window != NULL){
		free(inst->pitch_window);
	}
	if(inst->pitch_padded != NULL){
		free(inst->pitch_padded);
	}
	if(inst->pitch_spacing != NULL){
		free(inst->pitch_spacing);
	}
	if(inst->pitch_strategy != NULL){
		free(inst->pitch_strategy);
	}
	if(inst->silence_window != NULL){
		free(inst->silence_window);
	}
	if(inst->silence_spacing != NULL){
		free(inst->silence_spacing);
	}
	if(inst->silence_strategy != NULL){
		free(inst->silence_strategy);
	}
	free(inst);
}

struct Midi* me_process(float *input, audioInfo info, struct me_data *inst,
			int * exit_code)
{
	struct Midi* midi = ExtractMelody(
		&input, info,
		inst->pitch_window, inst->pitch_padded,
		inst->pitch_spacing, inst->pitch_strategy,
		inst->silence_window, inst->silence_spacing,
		inst->silence_mode, inst->silence_strategy,
		inst->hps, inst->tuning, inst->verbose, inst->prefix,
		exit_code);

	return midi;
}
