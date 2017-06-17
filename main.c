#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include "sndfile.h"
#include "comparison.h"
#include "pitchStrat.h"
#include "onsetStrat.h"


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
 *   --onset_window: number of frames of audiodata taken for each stft window for onset detection. def = 512
 *   --onset_padded: final size of stft window for onset detection after zero padding. 
 *                    if not set, the window size will be --onset_window.
 *                    cannot be set to less than --onset_window. def = -1
 *   --onset_spacing: stft window spacing for onset detection, def = 256
 *   --onset_strategy: strategy for onset detection, only OnsetsDS, def = OnsetsDS
 *
 *   -h: number of harmonic product specturm overtones, def = 2
 *   -p: prefix for fname where spectral data is stored, def = NULL;
 *
 * note that arge --pitch_window, --pitch_padded, --pitch_spacing,
 *  --onset_window, --onset_padded, --onset_spacing can be specified as:
 *           number of frames (ex: '4096')
 *           lenght of time (ex: '60ms', note that '60 ms' fails)
 */

const char* ERR_INVALID_FILE = "Audio file could not be opened for processing\n";
const char* ERR_FILE_NOT_MONO = "Input file must be Mono."
                          " Multi-channel audio currently not supported.\n";

int ReadAudioFile(char* inFile, float** buf, SF_INFO* info){
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

int ConvertToFrames(char* buf, int samplerate){
	char* endptr;
	int num;
	long longnum = strtol(buf, &endptr, 10);
	if (endptr == buf){
		return 0;
	}
	if (longnum > INT_MAX || longnum < INT_MIN){
		return 0;
	}
	num = longnum;
	if (*endptr!= '\0'){ //characters exist after the parsed number
		if(strcmp(endptr, "ms") == 0){ //proper millisecond format
			num = msToFrames(num, samplerate);
		}
		else{ //invalid format
			return 0;
		}
	}

	return num;
}

int main(int argc, char ** argv)
{
	char* inFile = NULL;
	char* outFile = NULL;

	//args for pitch detection
	char* p_windowsizeBuf = "4096";
	char* p_paddedsizeBuf = "none";
	char* p_spacingBuf = "2048";
	int p_windowsize, p_paddedsize, p_spacing;
	PitchStrategyFunc p_Strategy = &HPSDetectionStrategy; //HPS is default strategy
	
	//args for onset detection
	char* o_windowsizeBuf = "512";
	char* o_paddedsizeBuf = "none"; //-1 means no zero padding
	char* o_spacingBuf = "256";
	int o_windowsize, o_paddedsize, o_spacing;
	OnsetStrategyFunc o_Strategy = &OnsetsDSDetectionStrategy; //HPS is default strategy

	int hpsOvertones = 2;
	int verbose = 0; 
	char* prefix = NULL;

	SF_INFO info;
	float* input;

	int readFile = 0;
	
	//check command line arguments
	static struct option long_options[] =
		{
			{"pitch_window", required_argument, 0, 'a'},
			{"pitch_padded", required_argument, 0, 'x'},
			{"pitch_spacing", required_argument, 0, 'b'},
			{"pitch_strategy", required_argument, 0, 'c'},

			{"onset_window", required_argument, 0, 'd'},
			{"onset_padded", required_argument, 0, 'y'},
			{"onset_spacing", required_argument, 0, 'e'},
			{"onset_strategy", required_argument, 0, 'f'},

			{0,0,0,0},
		};

	int option_index = 0;

	int opt = 0;
	int badargs = 0;

	while ((opt = getopt_long (argc, argv, "i:o:vh:p:", long_options, &option_index)) != -1) {
		switch (opt) {
		case 'i':
			inFile = strdup(optarg);
			break;
		case 'o':
			outFile = strdup(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'a':
			p_windowsizeBuf = strdup(optarg);
			break;
		case 'x':
			p_paddedsizeBuf = strdup(optarg);
			break;
		case 'b':
			p_spacingBuf = strdup(optarg);
			break;
		case 'c':
			p_Strategy = choosePitchStrategy(optarg);
			if (p_Strategy == NULL){
				printf("Argument for option --pitch_strategy must be \"HPS\", \"BaNa\", or \"BaNaMusic\"\n");
				badargs = 1;
			}
			break;
		case 'd':
			o_windowsizeBuf = strdup(optarg);
			break;
		case 'y':
			o_paddedsizeBuf = strdup(optarg);
			break;
		case 'e':
			o_spacingBuf = strdup(optarg);
			break;
		case 'f':
			o_Strategy = chooseOnsetStrategy(optarg);
			if (o_Strategy == NULL){
				printf("Argument for option --onset_strategy must be \"OnsetsDS\"\n");
				badargs = 1;
			}
			break;
		case 'h':
			hpsOvertones = atoi(optarg);
			if(hpsOvertones < 1){
				printf("Argument for option -s must be a positive integer\n");
				badargs = 1;
			}
			break;
		case 'p':
			prefix = strdup(optarg);
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
	else if(!ReadAudioFile(inFile, &input, &info)){
		badargs = 1;
	}
	else{ //ReadAudioFile() succeeded
		readFile = 1;
		//determine values for p_windowsize, p_spacing, and p_paddedsize
		p_windowsize = ConvertToFrames(p_windowsizeBuf, info.samplerate);
		if(p_windowsize < 1){
			printf("Invalid arg for option --pitch_window\n");
			badargs = 1;
		}
		p_spacing = ConvertToFrames(p_spacingBuf, info.samplerate);
		if(p_spacing < 1){
			printf("Invalid arg for option --pitch_spacing\n");
			badargs = 1;
		}
		if(strcmp(p_paddedsizeBuf, "none") == 0){
			p_paddedsize = p_windowsize;
		}
		else{
			p_paddedsize = ConvertToFrames(p_paddedsizeBuf, info.samplerate);
		}
		if(p_paddedsize < 1){
			printf("Invalid arg for option --pitch_padded\n");
			badargs = 1;
		}
		else if(p_paddedsize < p_windowsize){
			printf("--pitch_padded cannot be set less than --pitch_windowsize, whose value is %d\n", p_windowsize);
			badargs = 1;
		}

		//determine values for o_windowsize, o_spacing, and o_paddedsize
		o_windowsize = ConvertToFrames(o_windowsizeBuf, info.samplerate);
		if(o_windowsize < 1){
			printf("Invalid arg for option --onset_window\n");
			badargs = 1;
		}
		o_spacing = ConvertToFrames(o_spacingBuf, info.samplerate);
		if(o_spacing < 1){
			printf("Invalid arg for option --onset_spacing\n");
			badargs = 1;
		}
		if(strcmp(o_paddedsizeBuf, "none") == 0){
			o_paddedsize = o_windowsize;
		}
		else{
			o_paddedsize = ConvertToFrames(o_paddedsizeBuf, info.samplerate);
		}	
		if(o_paddedsize < 1){
			printf("Invalid arg for option --onset_padded\n");
			badargs = 1;
		}
		else if(o_paddedsize < o_windowsize){
			printf("--onset_padded cannot be set less than --onset_windowsize, whose value is %d\n", o_windowsize);
			badargs = 1;
		}
	}


	if(!badargs){

		ExtractMelody(&input, info, outFile, 
				p_windowsize, p_paddedsize, p_spacing, p_Strategy, 
				o_windowsize, o_paddedsize, o_spacing, o_Strategy, 
				hpsOvertones, verbose, prefix);
	}
	if(readFile){
		free(input);
	}
	
	return 0;
}
