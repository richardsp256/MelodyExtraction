#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include "comparison.h"
#include "detectionStrat.h"

/* Usage is as follows:\n"
 * mandatory args:\n"
 *   -i: Input .wav file\n"
 *   -o: output .wav file\n"
 * optional args:\n"
 *   -v: verbose output\n"
 *   -w: stft window size, def = 2048\n"
 *   -s: stft window interval spacing, def = 128\n"
 *   -h: number of harmonic product specturm overtones, def = 2\n";
 *   -p: prefix for fname where spectral data is stored, def = NULL\n";
 *   -d: fundamental detection method - allowed arguments are HPS, BaNa, and BaNaMusic, def = HPS\n";
 */

int main(int argc, char ** argv)
{
	char* inFile = NULL;
	char* outFile = NULL;
	int p_windowsize = 4096;
	int p_spacing = 2048;
	PitchDetectionStrategyFunc p_Strategy = &HPSDetectionStrategy; //HPS is default strategy
	int hpsOvertones = 2;
	int verbose = 0; 
	char* prefix = NULL;
	
	//check command line arguments
	static struct option long_options[] =
		{
			{"pitch_window", required_argument, 0, 'a'},
			{"pitch_spacing", required_argument, 0, 'b'},
			{"pitch_strategy", required_argument, 0, 'c'},
			{0,0,0,0},
		};

	int option_index = 0;

	int opt = 0;
	int badargs = 0;

	while ((opt = getopt_long (argc, argv, "i:o:vh:p:d:", long_options, &option_index)) != -1) {
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
			p_Strategy = chooseStrategy(optarg);
			if (p_Strategy == NULL){
				printf("Argument for option --pitch_strategy must be a \"HPS\", \"BaNa\", or \"BaNaMusic\"\n");
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
		ExtractMelody(inFile, outFile, p_windowsize, p_spacing, hpsOvertones, verbose, prefix, p_Strategy);
	}

	return 0;
}
