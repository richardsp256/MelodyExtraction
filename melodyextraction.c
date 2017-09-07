#include "melodyextraction.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
//#include <unistd.h>

#include "comparison.h"
#include "sndfile.h"
#include "pitchStrat.h"
#include "onsetStrat.h"
#include "silenceStrat.h"

struct me_data{
	char * prefix;
	char * pitch_window;
	char * pitch_padded;
	char * pitch_spacing;
	PitchStrategyFunc pitch_strategy;
	char * onset_window;
	char * onset_padded;
	char * onset_spacing;
	OnsetStrategyFunc onset_strategy;
	char * silence_window;
	char * silence_spacing;
	SilenceStrategyFunc silence_strategy;
	int silence_mode;
	int h;
	int t;
	int v;
};

//default arg settings:
char* PITCH_WINDOW_DEF = "4096";
char* PITCH_PADDED_DEF = "none"; //none will set pitch_padded to pitch_window
char* PITCH_SPACING_DEF = "2048";
PitchStrategyFunc PITCH_STRATEGY_DEF = &HPSDetectionStrategy;

char* ONSET_WINDOW_DEF = "512";
char* ONSET_PADDED_DEF = "none"; //none will set onset_padded to onset_window
char* ONSET_SPACING_DEF = "256";
OnsetStrategyFunc ONSET_STRATEGY_DEF = &OnsetsDSDetectionStrategy;

char* SILENCE_WINDOW_DEF = "10ms";
char* SILENCE_SPACING_DEF = "none"; //-1 means to set equal to s_windowsize
int SILENCE_MODE_DEF = 0;
SilenceStrategyFunc SILENCE_STRATEGY_DEF = &fVADDetectionStrategy;

char* ERR_INVALID_FILE = "Audio file could not be opened for processing\n";
char* ERR_FILE_NOT_MONO = "Input file must be Mono."
                          " Multi-channel audio currently not supported.\n";

struct me_data* me_data_new(){
	struct me_data *inst = malloc(sizeof(struct me_data));
	if (inst != NULL){
		if (me_data_reset(inst) != 0){
			me_data_free(inst);
		}
	}
	return inst;
}

void me_data_free(struct me_data* inst){
	free(inst);
}

int me_data_reset(struct me_data * inst){
	inst->prefix = NULL;
	if (me_set_pitch_window(inst,PITCH_WINDOW_DEF) == 1){
		return 1;
	}
	if (me_set_pitch_padded(inst,PITCH_PADDED_DEF) == 1){
		return 1;
	}
	if (me_set_pitch_spacing(inst,PITCH_SPACING_DEF) ==1){
		return 1;
	}
	inst->pitch_strategy = PITCH_STRATEGY_DEF;	
	if (me_set_onset_window(inst,ONSET_WINDOW_DEF) == 1){
		return 1;
	}
	if (me_set_onset_padded(inst,ONSET_PADDED_DEF) == 1){
		return 1;
	}
	if (me_set_onset_spacing(inst,ONSET_SPACING_DEF) ==1){
		return 1;
	}
	inst->onset_strategy = ONSET_STRATEGY_DEF;
	if (me_set_silence_window(inst,SILENCE_WINDOW_DEF) == 1){
		return 1;
	}
	me_set_silence_mode(inst,SILENCE_MODE_DEF);
	if (me_set_silence_spacing(inst,SILENCE_SPACING_DEF) ==1){
		return 1;
	}
	inst->silence_strategy = SILENCE_STRATEGY_DEF;
	me_set_hps_overtones(inst,2);
	me_set_tuning(inst,0);
	me_set_verbose(inst,0);
	return 0;
}

int set_string_field_helper(char ** field, char * value){
	// this is to help set fields of me_data
	// return 1 if there is a malloc error
	// otherwise, return 0
	if (*field != NULL){
		free(*field);
	}
	*field = strdup(value);
	if (*field == NULL){
		return 1;
	}
	return 0;
}

int me_set_prefix(struct me_data* inst,char* value){
	return set_string_field_helper(&(inst->prefix), value);
}

int me_set_pitch_window(struct me_data* inst,char* value){
	return set_string_field_helper(&(inst->pitch_window), value);
}

int me_set_pitch_padded(struct me_data* inst,char* value){
	return set_string_field_helper(&(inst->pitch_padded), value);
}

int me_set_pitch_spacing(struct me_data* inst,char* value){
	return set_string_field_helper(&(inst->pitch_spacing), value);
}

int me_set_pitch_strategy(struct me_data* inst,char* value){
	PitchStrategyFunc p_Strategy = choosePitchStrategy(value);
	if (p_Strategy != NULL){
		inst->pitch_strategy = p_Strategy;
		return 0;
	}
	return 2;
}

int me_set_onset_window(struct me_data* inst,char* value){
	return set_string_field_helper(&(inst->onset_window), value);
}

int me_set_onset_padded(struct me_data* inst,char* value){
	return set_string_field_helper(&(inst->onset_padded), value);
}

int me_set_onset_spacing(struct me_data* inst,char* value){
	return set_string_field_helper(&(inst->onset_spacing), value);
}

int me_set_onset_strategy(struct me_data* inst,char* value){
	OnsetStrategyFunc o_Strategy = chooseOnsetStrategy(value);
	if (o_Strategy != NULL){
		inst->onset_strategy = o_Strategy;
		return 0;
	}
	return 2;
}

int me_set_silence_window(struct me_data* inst, char* value){
	return set_string_field_helper(&(inst->silence_window), value);
}

int me_set_silence_spacing(struct me_data* inst, char* value){
	return set_string_field_helper(&(inst->silence_spacing), value);
}

int me_set_silence_strategy(struct me_data* inst, char* value){
	SilenceStrategyFunc s_Strategy = chooseSilenceStrategy(value);
	if (s_Strategy != NULL){
		inst->silence_strategy = s_Strategy;
		return 0;
	}
	return 2;
}

int me_set_silence_mode(struct me_data* inst,int value){
	if ((value < 0) || (value > 3)){ 
		return 2;
	}
	inst->silence_mode = value;
	return 0;
}

int me_set_hps_overtones(struct me_data* inst,int value){
	if (value>0){ 
		inst->h = value;
		return 0;
	}
	return 2;
}

int me_set_tuning(struct me_data* inst,int value){
	if ((value < 0) || (value > 2)){ 
		return 2;
	}
	inst->t = value;
	return 0;
}

int me_set_verbose(struct me_data* inst,int value){
	if ((value == 0) || (value == 1)){ 
		inst->v = value;
		return 0;
	}
	return 1;
}

int ReadAudioFile(char* inFile, float** buf, SF_INFO* info)
{
	SNDFILE * f = sf_open(inFile, SFM_READ, info);
	if( !f ){
		printf("%s", ERR_INVALID_FILE);
		return 0;
	}
	if((*info).channels != 1){
		printf("%s", ERR_FILE_NOT_MONO);
		sf_close( f );
		return 0;
	}

	(*buf) = malloc( sizeof(float) * (*info).frames);
	sf_readf_float( f, (*buf), (*info).frames );
	sf_close( f );

	return 1;
}

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

struct Midi* me_process(float **input, SF_INFO info, struct me_data *inst)
{
	char* prefix = NULL;
	
	int p_windowsize, p_paddedsize, p_spacing;
	PitchStrategyFunc p_Strategy;

	int o_windowsize, o_paddedsize, o_spacing;
	OnsetStrategyFunc o_Strategy;

	int s_windowsize, s_spacing, s_mode;
	SilenceStrategyFunc s_Strategy;

	int hpsOvertones, verbose, tuning;

	int badargs = 0;

	// set the values directly from inst
	prefix = inst->prefix;
	p_Strategy = inst->pitch_strategy;
	o_Strategy = inst->onset_strategy;
	s_Strategy = inst->silence_strategy;
	s_mode = inst->silence_mode;
	hpsOvertones = inst->h;
	verbose = inst->v;
	tuning = inst->t;

	
	//determine values for p_windowsize, p_spacing, and p_paddedsize
	p_windowsize = ConvertToFrames(inst->pitch_window,
				       info.samplerate);
	if(p_windowsize < 1){
		printf("Invalid arg for option pitch_window\n");
		badargs = 1;
	}
	
	p_spacing = ConvertToFrames(inst->pitch_spacing,
				    info.samplerate);
	if(p_spacing < 1){
		printf("Invalid arg for option pitch_spacing\n");
		badargs = 1;
	}
	if(strcmp(inst->pitch_padded, "none") == 0){
		p_paddedsize = p_windowsize;
	}
	else{
		p_paddedsize = ConvertToFrames(inst->pitch_padded,
					       info.samplerate);
	}
	if(p_paddedsize < 1){
		printf("Invalid arg for option pitch_padded\n");
		badargs = 1;
	}
	else if(p_paddedsize < p_windowsize){
		printf("pitch_padded cannot be set less than pitch_windowsize, whose value is %d\n", p_windowsize);
		badargs = 1;
	}

	//determine values for o_windowsize, o_spacing, and o_paddedsize
	o_windowsize = ConvertToFrames(inst->onset_window,
				       info.samplerate);
	if(o_windowsize < 1){
		printf("Invalid arg for option onset_window\n");
		badargs = 1;
	}
	o_spacing = ConvertToFrames(inst->onset_spacing,
				    info.samplerate);
	if(o_spacing < 1){
		printf("Invalid arg for option onset_spacing\n");
		badargs = 1;
	}
	if(strcmp(inst->onset_padded, "none") == 0){
		o_paddedsize = o_windowsize;
	}
	else{
		o_paddedsize = ConvertToFrames(inst->onset_padded,
					       info.samplerate);
	}	
	if(o_paddedsize < 1){
		printf("Invalid arg for option onset_padded\n");
		badargs = 1;
	}
	else if(o_paddedsize < o_windowsize){
		printf("onset_padded cannot be set less than onset_windowsize, whose value is %d\n", o_windowsize);
		badargs = 1;
	}

	//determine values for s_windowsize and s_spacing
	if (strcmp(inst->silence_window,"10ms") == 0){
		s_windowsize = 10;
	} else if (strcmp(inst->silence_window,"20ms") == 0){
		s_windowsize = 20;
	} else if (strcmp(inst->silence_window,"30ms") == 0){
		s_windowsize = 30;
	} else {
		printf("silence_window can only be \"10ms\", \"20ms\", or \"30ms\"\n");
		s_windowsize = 1; //just give it a temp value so s_spacing doesnt fail to set
		badargs = 1;
	}

	if (strcmp(inst->silence_spacing,"none") == 0){
		s_spacing = s_windowsize;
	} else {
		if (numParser(inst->silence_spacing, &s_spacing)==0) {
			badargs = 1;
			printf("silence_spacing must be in units of ms\n");
		}
	}

	if(s_spacing < 1){
		printf("Invalid arg for option silence_spacing\n");
		badargs = 1;
	}

	struct Midi* midi = NULL;
	
	if(!badargs){
		midi = ExtractMelody(input, info, p_windowsize, p_paddedsize,
			       p_spacing, p_Strategy, o_windowsize,
			       o_paddedsize, o_spacing, o_Strategy,
			       s_windowsize, s_spacing, s_mode,
			       s_Strategy, hpsOvertones, tuning,
			       verbose, prefix);
	}

	return midi;
}
