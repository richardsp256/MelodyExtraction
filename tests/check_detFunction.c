#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../src/simpleDetFilter.h"
#include "doubleArrayTesting.h"

void float_to_double_array(float* array, int length, double** dblarray){

}

void double_to_float_array(double* array, int length, float** fltarray){
}

int rollSigmaTestTemplate(int *intInput, int intInputLen, double *dblInput,
			  int dblInputLen, char *strInput, double **array){
	/* this is the template for testing rollSigma.

	   There is asumed to be 4 intInput arguments and 1 dblInput argument.
	   strInput is assumed to be the file name from which the input is 
	   loaded.

	   The result must be converted from a float array to a double array.
	 */
	ck_assert_int_eq(intInputLen,4);
	ck_assert_int_eq(dblInputLen,1);

	int numWindows = intInput[3]
	
	/* load the input array. */

	int input_length;
	double *original_input;

	input_length = readDoubleArray(strInput, 1, original_input);

	float *buffer;

	double_to_float_array(original_input, input_length, &buffer);

	free(original_input);

	float *original_sigma = malloc(sizeof(float)*numWindows);

	rollSigma(intInput[0], intInput[1], dblInput[0], intInput[2],
		  input_length, numWindows, buffer, &original_sigma);
	free(buffer);

	float_to_double_array(original_sigma, numWindows, &buffer);

	free(original_sigma);

	return numWindows;
}
