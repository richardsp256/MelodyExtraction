#include <stdio.h>
#include <stdlib.h>
#include "comparison.h"

char* ERR_BAD_ARGS = "Incorrect command line args. Usage is as follows:\n"
				 "  [1] Input .wav file\n"
				 "  [2] output .wav file\n";

int main(int argc, char ** argv)
{
	//check command line arguments
	if(argc != 3){
		printf("%s", ERR_BAD_ARGS);
		exit(-1);
	}
	char* inFile = argv[1];
	char* outFile = argv[2];

	ExtractMelody(inFile, outFile);

	return 0;
}