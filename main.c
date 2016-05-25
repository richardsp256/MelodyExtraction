#include <stdio.h>
#include <stdlib.h>
#include "comparison.h"

char* BAD_ARGS = "ERROR: incorrect command line args. Usage is as follows:\n"
				 "  [1] Input .wav file\n";

int main(int argc, char ** argv)
{
	//check command line arguments
	if(argc != 2){
		printf("%s",BAD_ARGS);
		exit(-1);
	}
	char* fname = argv[1];

	ExtractMelody(fname);

	return 0;
}