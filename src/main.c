#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include "melodyextraction.h"
#include "io_wav.h"

/* Usage is as follows:
 * mandatory args:
 *   -i: Input .wav file
 *   -o: output .wav file
 * optional args:
 *   -v: verbose output
 *
 *   --pitch_window: number of frames of audiodata taken for each stft window for pitch detection. def = 4096
 *   --pitch_padded: final size of stft window for pitch detection after zero padding. 
 *                    if not set, the window size will be --pitch_window.
 *                    cannot be set to less than --pitch_window. def = -1
 *   --pitch_spacing: stft window spacing for pitch detection, def = 2048
 *   --pitch_strategy: strategy for pitch detection, either HPS, BaNa, and BaNaMusic, def = HPS
 *
 *   --silence_window: number of milliseconds of audio data in a frame 
 *                      considered for silence detection. The only allowed 
 *                      values are 10ms, 20ms, and 30ms, def=10ms
 *   --silence_spacing: spacing between windows considered for silence 
 *                       detection. This must be in units of ms. If this is -1, 
 *                       it is set to the value of --silence_window, def = -1 
 *   --silence_strategy: strategy for silence detection, only fVAD, def = fVAD
 *   --silence_mode: The mode to run fVAD in. The valid values are 0 
 *                    ("quality"), 1 ("low bitrate"), 2 ("aggressive"), and 3
 *                    ("very aggressive"), def = 0
 *
 *   -h: number of harmonic product specturm overtones, def = 2
 *   -t: tuning adjustment mode. 0 = no adjustment,  1 = adjust with threshold,  2 = always adjust
 *   -p: prefix for fname where spectral data is stored, def = NULL;
 *
 * note that arge --pitch_window, --pitch_padded, --pitch_spacing,
 *  --onset_window, --onset_padded, --onset_spacing can be specified as:
 *           number of frames (ex: '4096')
 *           lenght of time (ex: '60ms', note that '60 ms' fails)
 */


int main(int argc, char ** argv)
{
	char* inFile = NULL;
	char* outFile = NULL;

	//check command line arguments
	static struct option long_options[] =
		{
			{"pitch_window", required_argument, 0, 'a'},
			{"pitch_padded", required_argument, 0, 'x'},
			{"pitch_spacing", required_argument, 0, 'b'},
			{"pitch_strategy", required_argument, 0, 'c'},

			{"silence_window", required_argument, 0, 'g'},
			{"silence_spacing", required_argument, 0, 'j'},
			{"silence_strategy", required_argument, 0, 'k'},
			{"silence_mode", required_argument, 0, 'l'},

			{0,0,0,0},
		};

	int option_index = 0;

	int opt = 0;
	int badargs = 0;

	struct me_settings *settings= me_settings_new();

	while ((opt = getopt_long (argc, argv, "i:o:vt:h:p:", long_options, &option_index)) != -1) {
		switch (opt) {
		case 'i':
			inFile = strdup(optarg);
			break;
		case 'o':
			outFile = strdup(optarg);
			break;
		case 'v':
			settings->verbose = 1;
			break;
		case 't':
			settings->tuning = atoi(optarg);
			break;
		case 'a':
			settings->pitch_window = strdup(optarg);
			break;
		case 'x':
			settings->pitch_padded = strdup(optarg);
			break;
		case 'b':
			settings->pitch_spacing = strdup(optarg);
			break;
		case 'c':
			settings->pitch_strategy = strdup(optarg);
			break;
		case 'g':		
			settings->silence_window = strdup(optarg);
			break;
		case 'j':
			settings->silence_spacing = strdup(optarg);
			break;
		case 'k':
			settings->silence_strategy = strdup(optarg);
			break;
		case 'l':
			settings->silence_mode = atoi(optarg);
			break;
		case 'h':
			settings->hps = atoi(optarg);
			break;
		case 'p':
			settings->prefix = strdup(optarg);
			break;
		case '?':
			badargs = 1;
			break;
		default:
			badargs = 1;
			break;
		}
	}

	if(outFile == NULL){
		printf("Mandatory Argument -o not set\n");
		badargs = 1;
	}

	if(inFile == NULL){
		printf("Mandatory Argument -i not set\n");
		badargs = 1;
	}

	if(!badargs){

		audioInfo info;

		float* input;
		int exit_code = ReadAudioFile(inFile, &input, &info,
					      settings->verbose);
		if (exit_code != 0){
			printf("Error while reading audio from %s\n",
			       inFile);
			printf("Error msg: %s\n", me_strerror(exit_code));
			exit(EXIT_FAILURE);
		}

		struct me_data *inst;
		char* err = me_data_init(&inst, settings, info);
		//printf("val of inst: %d  %d  %d\n", inst, &inst, *inst);
		if(inst == NULL){
			printf("error initializing me_data: %s\n", err);
			me_settings_free(settings);
			free(input);
			exit(EXIT_FAILURE);
		}

		//printf("%d  %d  %d\n", inst->pitch_window, inst->pitch_padded, inst->pitch_spacing);
		//printf("%d  %d  %d\n", inst->onset_window, inst->onset_padded, inst->onset_spacing);
		//printf("%d  %d  %d\n", inst->silence_window, inst->silence_mode, inst->silence_spacing);
		//printf("%d  %d  %d\n", inst->hps, inst->tuning, inst->verbose);

		struct Midi* midi = me_process(input, info, inst, &exit_code);

		me_settings_free(settings);
		me_data_free(inst);
		free(input);

		if ((midi == NULL) || (exit_code != 0)){
			if (exit_code == 0){
				printf("Unexpected behavior: exit_code indicates success and output is NULL\n");
				exit(EXIT_FAILURE);
			}

			const char * msg = me_strerror(exit_code);
			if (msg == NULL){
				printf("Unexpected exit code: %d\n", exit_code);
			} else {
				printf("Error Msg: %s\n",msg);
			}

			if (midi != NULL){
				printf("Unexpected Behavior: midi data is not NULL\n");
			}
			exit(EXIT_FAILURE);
		}

		if (SaveMIDI(midi, outFile, 1)){
			printf("Error writing midi to %s\n", outFile);
			exit(EXIT_FAILURE);
		}
		freeMidi(midi);
	}
	
	return 0;
}
