#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
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

int main(int argc, char ** argv)
{
	char* inFile = NULL;
	char* outFile = NULL;

	//args for pitch detection
	int p_windowsize = 4096;
	int p_paddedsize = -1; //-1 means no zero padding
	int p_spacing = 2048;
	PitchStrategyFunc p_Strategy = &HPSDetectionStrategy; //HPS is default strategy
	
	//args for onset detection
	int o_windowsize = 512;
	int o_paddedsize = -1; //-1 means no zero padding
	int o_spacing = 256;
	OnsetStrategyFunc o_Strategy = &OnsetsDSDetectionStrategy; //HPS is default strategy

	int hpsOvertones = 2;
	int verbose = 0; 
	char* prefix = NULL;

	SF_INFO info;
	float* input;
	
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
			inFile = optarg;
			break;
		case 'o':
			outFile = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'a':
			p_windowsize = atoi(optarg);
			if(p_windowsize < 1){
				printf("Argument for option --pitch_window must be a positive integer\n");
				badargs = 1;
			}
			break;
		case 'x':
			p_paddedsize = atoi(optarg);
			if(p_paddedsize < 1){
				printf("Argument for option --pitch_padded must be a positive integer\n");
				badargs = 1;
			}
			break;
		case 'b':
			p_spacing = atoi(optarg);
			if(p_spacing < 1){
				printf("Argument for option --pitch_spacing must be a positive integer\n");
				badargs = 1;
			}
			break;
		case 'c':
			p_Strategy = choosePitchStrategy(optarg);
			if (p_Strategy == NULL){
				printf("Argument for option --pitch_strategy must be \"HPS\", \"BaNa\", or \"BaNaMusic\"\n");
				badargs = 1;
			}
			break;
		case 'd':
			o_windowsize = atoi(optarg);
			if(o_windowsize < 1){
				printf("Argument for option --onset_window must be a positive integer\n");
				badargs = 1;
			}
			break;
		case 'y':
			o_paddedsize = atoi(optarg);
			if(o_paddedsize < 1){
				printf("Argument for option --onset_padded must be a positive integer\n");
				badargs = 1;
			}
			break;
		case 'e':
			o_spacing = atoi(optarg);
			if(o_spacing < 1){
				printf("Argument for option --onset_spacing must be a positive integer\n");
				badargs = 1;
			}
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
			prefix = optarg;
			break;
		case '?':
			badargs = 1;
			break;
		default:
			badargs = 1;
			break;
		}
	}
	if(inFile == NULL){
		printf("Mandatory Argument -i not set\n");
		badargs = 1;
	}
	if(outFile == NULL){
		printf("Mandatory Argument -o not set\n");
		badargs = 1;
	}
	if(p_paddedsize == -1){
		p_paddedsize = p_windowsize;
	}
	if(p_paddedsize < p_windowsize){
		printf("--pitch_padded cannot be set less than --pitch_windowsize, whose value is %d\n", p_windowsize);
		badargs = 1;
	}
	if(o_paddedsize == -1){
		o_paddedsize = o_windowsize;
	}
	if(o_paddedsize < o_windowsize){
		printf("--onset_padded cannot be set less than --onset_windowsize, whose value is %d\n", o_windowsize);
		badargs = 1;
	}

	if(!badargs){
		//reads in .wav
		if(!ReadAudioFile(inFile, &input, &info)){
			return -1;
		}

		ExtractMelody(&input, info, outFile, 
				p_windowsize, p_paddedsize, p_spacing, p_Strategy, 
				o_windowsize, o_paddedsize, o_spacing, o_Strategy, 
				hpsOvertones, verbose, prefix);

		free(input);
	}

	return 0;
}
