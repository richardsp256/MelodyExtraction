#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "doubleArrayTesting.h"


int isLittleEndian()
{
	// From https://stackoverflow.com/a/3877387
	short int number = 0x1;
	char *numPtr = (char*)&number;
	return (numPtr[0] == 1);
}

int checkSystemFileCompatibility()
{
	// Checks that the system can actually run the tests
	// for now the system must be little endian (later on, we can convert
	// between endianness if necessary)
	// also need to make sure sizeof(int) == 2, sizeof(long) == 4,
	// sizeof(float) == 2, and sizeof(double) == 4

	if (isLittleEndian() != 1){
		printf("Can't read files because system is Big Endian\n");
		return 0;
	}

	if ((sizeof(int) != 4) || (sizeof(long) != 4) || (sizeof(float) !=4)
	    || (sizeof(double) != 4)){
		printf(("Can't properly run tests because dtypes don't meet "
			"standard sizes.\nThese standard sizes include:\n"
			"int    : 4 bytes\n"
			"long   : 4 bytes\n"
			"float  : 4 bytes\n"
			"double : 4 bytes\n"));
		return 0;
	}
	return 1;
}

int readDoubleArray(char *fileName, int system_little_endian, double *array){
	/* Reads in a little-endian double array from a binary file. Returns 
	 * the size of the array. Sets array to point to the loaded values.
	 *
	 * system_little_endian indicates whether or not the system is big 
	 * endian (0) or little endian (1).
	 */

	FILE *fp;
	

	fp = fopen(fileName,"rb");
	if (fp == NULL){
		printf("Could not find file %s\n", fileName);
		return -1;
	}

	/* Identify the size of the array https://stackoverflow.com/a/238607 */
	fseek(fp, 0L, SEEK_END);
	long num_bytes = ftell(fp);
	long temp = (num_bytes/sizeof(double));

	if (num_bytes % sizeof(double)){
		printf("The file does not have the correct number of bytes\n");
		return -2;
	}
	
	if (temp > INT_MAX){
		printf("The array is too long\n");
		return -3;
	}
	int length = (int)temp;
	rewind(fp);
	array = malloc(sizeof(double)*length);
	if (array == NULL){
		return -4;
	}
	fread(array, sizeof(double), length, fp);
	fclose(fp);
	return length;
}


void compareArrayEntries(double *ref, double* other, int length,
			 double tol, int rel,double abs_zero_tol)
{
	// if relative < 1, then we compare absolute value of the absolute
	// difference between values
	// if relative > 1, we need to specially handle when the reference
	// value is zero. In that case, we look at the absolute differce and
	// compare it to abs_zero_tol
	
	int i;
	double diff;

	for (i = 0; i< length; i++){
		diff = abs(ref[i]-other[i]);
		if (rel >=1){
			if (ref[i] == 0){
				// we will just compute abs difference
				ck_assert_msg(diff <= abs_zero_tol,
					      ("ref[%d] = 0, comp[%d] = %e"
					       " has abs diff > %e\n"),
					      i, i, other[i],
					      abs_zero_tol);
				continue;
			}
			diff = diff/abs(ref[i]);
		}
		if (diff>tol){
			if (rel>=1){
				ck_abort_msg(("ref[%d] = %e, comp[%d] = %e"
					      " has rel dif > %e"),
					     i,ref[i],i,other[i], tol);
			} else {
				ck_abort_msg(("ref[%d] = %e, comp[%d] = %e"
					      " has abs dif > %e"),
					     i,ref[i],i,other[i], tol);
			}
		}		
	}
}


void process_double_array_test(struct dblArrayTestEntry entry, double tol,
			       int rel, double abs_zero_tol){
	/* run the double array test. */
	double *calc = NULL;
	int calc_len;
	double *ref = NULL;
	int ref_len;

	if (isLittleEndian() == 1){
		ck_abort_msg(("Currently unable to run test on Big Endian "
			      "machine.\n"));
	}

	// Load in the reference array
	ref_len = readDoubleArray(entry.resultFname, 1, ref);
	if (ref_len<=0){
		ck_abort_msg(("Encountered an issue while determining "
			      "reference array.\n"));
	}

	// run the function to compute to determine the calculated array
	calc_len = entry.func(entry.intInput, entry.intInputLen, entry.dblInput,
			      entry.dblInputLen, entry.strInput, calc);
	ck_assert_int_eq(calc_len,ref_len);

	compareArrayEntries(ref, calc, ref_len, tol, rel, abs_zero_tol);
	free(calc);
	free(ref);
}
