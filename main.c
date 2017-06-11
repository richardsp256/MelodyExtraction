#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include "comparison.h"
#include "pitchStrat.h"
#include "onsetStrat.h"


/* Usage is as follows:\n"
 * mandatory args:\n"
 *   -i: Input .wav file\n"
 *   -o: output .wav file\n"
 * optional args:\n"
 *   -v: verbose output\n"
 *   --pitch_window: stft window size for pitch detection, def = 4096\n"
 *   --pitch_spacing: stft window spacing for pitch detection, def = 2048\n"
 *   --pitch_strategy: strategy for pitch detection, either HPS, BaNa, and BaNaMusic, def = HPS\n"
 *   --onset_window: stft window size for onset detection, def = 512\n"
 *   --onset_spacing: stft window spacing for onset detection, def = 256\n"
 *   --onset_strategy: strategy for onset detection, only OnsetsDS, def = OnsetsDS\n"
 *   -h: number of harmonic product specturm overtones, def = 2\n"
 *   -p: prefix for fname where spectral data is stored, def = NULL\n";
 */

int main(int argc, char ** argv)
{
	char* inFile = NULL;
	char* outFile = NULL;

	//args for pitch detection
	int p_windowsize = 4096;
	int p_spacing = 2048;
	PitchStrategyFunc p_Strategy = &HPSDetectionStrategy; //HPS is default strategy
	
	//args for onset detection
	int o_windowsize = 512;
	int o_spacing = 256;
	OnsetStrategyFunc o_Strategy = &OnsetsDSDetectionStrategy; //HPS is default strategy

	int hpsOvertones = 2;
	int verbose = 0; 
	char* prefix = NULL;
	
	//check command line arguments
	static struct option long_options[] =
		{
			{"pitch_window", required_argument, 0, 'a'},
			{"pitch_spacing", required_argument, 0, 'b'},
			{"pitch_strategy", required_argument, 0, 'c'},

			{"onset_window", required_argument, 0, 'd'},
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

	if(!badargs){
		ExtractMelody(inFile, outFile, p_windowsize, p_spacing, p_Strategy, o_windowsize, o_spacing, o_Strategy, hpsOvertones, verbose, prefix);
	}

	return 0;
}
