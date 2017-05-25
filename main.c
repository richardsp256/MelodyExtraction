#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "comparison.h"

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
 */

int main(int argc, char ** argv)
{
	char* inFile = NULL;
	char* outFile = NULL;
	int windowsize = 2048;
	int spacing = 1024;
	int hpsOvertones = 2;
	int verbose = 0; 
	char* prefix = NULL;

	//check command line arguments
	int opt = 0;
	int badargs = 0;
	while ((opt = getopt (argc, argv, "i:o:vw:s:h:p:")) != -1) {
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
		case 'w':
			windowsize = atoi(optarg);
			if(windowsize < 1){
				printf("Argument for option -w must be a positive integer\n");
				badargs = 1;
			}
			break;
		case 's':
			spacing = atoi(optarg);
			if(spacing < 1){
				printf("Argument for option -s must be a positive integer\n");
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
			if (optopt == 'i' || optopt == 'o' || optopt == 'w' || optopt == 's' || optopt == 'h')
				printf ("Option -%c requires an argument.\n", optopt);
			else
				printf ("Unknown option `-%c'.\n", optopt);
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
	  ExtractMelody(inFile, outFile, windowsize, spacing, hpsOvertones, verbose, prefix);
	}

	return 0;
}
