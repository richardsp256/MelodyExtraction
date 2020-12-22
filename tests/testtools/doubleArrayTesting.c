#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <float.h> // FLT_MIN, FLT_MAX
#include <check.h>
#include <math.h>


#include "doubleArrayTesting.h"
#include "../../src/utils.h" // isLittleEndian


// ROOT_DIR_PATH should be specified to the compiler and should indicate the
// path to the root directory of the repository
#ifndef ROOT_DIR_PATH
#error "The ROOT_DIR_PATH macro must be defined by the build system"
#endif

#define MAKE_STR_HELPER(x) #x
#define MAKE_STR(x) MAKE_STR_HELPER(x)
const char * const root_dir_path = MAKE_STR(ROOT_DIR_PATH);

#define FILE_BUFFER_LEN 512

// this opens a path that was given relative to the base file path
FILE * openTestFile(const char* path, const char* mode){
	char full_path[FILE_BUFFER_LEN];

	size_t root_dir_len = strlen(root_dir_path);
	// TODO: it would be safer to handle the case where base_section is not
	//       null-terminated
	size_t base_section_len = strlen(path);

	// add an extra byte for an extra '/' character
	// add an extra byte for null terminating character
	size_t combined_length = root_dir_len + base_section_len + 2;
	if (combined_length > FILE_BUFFER_LEN){
		printf(("The full path to the file requires %zd bytes. This "
			"exceeds the max default buffer size of %d bytes\n"),
		       combined_length, FILE_BUFFER_LEN);
		abort();
	}

	strcpy(full_path,root_dir_path);
	full_path[root_dir_len] = '/';
	strcpy(full_path+(root_dir_len+1),path);

	// finally, read the file!
	FILE *fp = fopen(full_path,mode);
	if (fp == NULL){
		printf("Could not find file %s\n", full_path);
	}
	return fp;
}

int getArrayLength(FILE *fp, int object_size){
	/* Finds the length of the array saved in a file. We assume that the 
	 * array has no more than INT_MAX objects. 
	 * https://stackoverflow.com/a/238607
	 *
	 * It assumes that the file starts from the begining.
	 */

	fseek(fp, 0L, SEEK_END);
	long num_bytes = ftell(fp);
	long temp = (num_bytes/object_size);

	if ((num_bytes % object_size) != 0){
		printf("The file does not have the correct number of bytes\n");
		return -2;
	}

	if (temp > INT_MAX){
		printf("The array is too long\n");
		return -3;
	}
	int length = (int)temp;
	rewind(fp);
	return length;
}

int readDoubleArray(const char *fileName, bool system_little_endian,
		    enum PrecisionEnum output_prec, void **array){
	if (!system_little_endian){
		printf("Doesn't currently support big-endian machines\n");
		abort();
	}

	FILE * fp = openTestFile(fileName, "rb");

	int length = getArrayLength(fp, sizeof(double));
	if (length < 0){
		return length;
	}

	bool range_problem = false;
	bool file_problem = false;

	switch(output_prec){
	case float_precision: {
		(*array) = malloc(sizeof(float)*length);
		float * buffer = (float *) (*array);
		if (buffer){
			for (int i = 0; i < length; i++){
				double val;
				if (1 != fread(&val, sizeof(double), 1, fp)){
					file_problem = true;
					break;
				}

				if (fabs(val) == 0.0f ||
				    ( (fabs(val) >= FLT_MIN) &&
				      (fabs(val) <= FLT_MAX))) {
					buffer[i] = (float) val;
				} else{
					printf("%e can't be represented as a "
					       "single precision float\n",
					       val);
					range_problem = true;
					break;
				}
			}
		}
		break;
	} // end of float_precision case
	case double_precision: {
		(*array) = malloc(sizeof(double)*length);
		if (*array){
			if ((size_t)length != fread((*array), sizeof(double),
						    length, fp)){
				file_problem = true;
			}
		}
		break;
	} // end of double_precision case
	} // end of switch statement
	fclose(fp);

	// Now to do some error handling
	if ((*array) == NULL){
		printf("There was an issue with malloc\n");
		return -4;
	} else if (range_problem){
		free(*array);
		printf("There was an issue with converting to the desired "
		       "precision.\n");
		return -5;
	} else if (file_problem){
		free(*array);
		printf("There was an issue reading from the file\n");
		return -6;
	}
	return length;
}


// if relative < 1, then we compare absolute value of the absolute
// difference between values
// if relative > 1, we need to specially handle when the reference
// value is zero. In that case, we look at the absolute differce and
// compare it to abs_zero_tol
#define create_compareArrayEntriesFunc(ftype)                                \
void compareArrayEntries_ ## ftype (const ftype *ref, const ftype* other,    \
				    int length,	ftype tol, int rel,          \
				    ftype abs_zero_tol)                      \
{                                                                            \
	for (int i = 0; i< length; i++){                                     \
		double diff = fabs(ref[i]-other[i]);                         \
		if (rel >=1){                                                \
			if (ref[i] == 0){                                    \
				/* we will just compute abs difference */    \
				ck_assert_msg(diff <= abs_zero_tol,          \
					      ("ref[%d] = 0, comp[%d] = %e"  \
					       " has abs diff > %e\n"),      \
					      i, i, other[i],                \
					      abs_zero_tol);                 \
				continue;                                    \
			}                                                    \
			diff = diff/fabs(ref[i]);                            \
		}                                                            \
		if (diff>tol){                                               \
			if (rel>=1){                                         \
				ck_abort_msg(("ref[%d] = %e, comp[%d] = %e"  \
					      " has rel dif > %e"),          \
					     i,ref[i],i,other[i], tol);      \
			} else {                                             \
				ck_abort_msg(("ref[%d] = %e, comp[%d] = %e"  \
					      " has abs dif > %e"),          \
					     i,ref[i],i,other[i], tol);      \
			}                                                    \
		}		                                             \
	}								     \
}

create_compareArrayEntriesFunc(double)
create_compareArrayEntriesFunc(float)

void compareAgainstSavedArray(const char* fname, const void* test, int test_len,
			      enum PrecisionEnum prec, double tol, bool rel,
			      double abs_zero_tol)
{
	/* run the double array test. */
	void *ref = NULL;
	int ref_len;

	if (!IsLittleEndian()){
		ck_abort_msg(("Currently unable to run test on Big Endian "
			      "machine.\n"));
	}

	// Load in the reference array
	ref_len = readDoubleArray(fname, true, prec, &ref);
	if (ref_len<=0){
		printf("%d\n",ref_len);
		ck_abort_msg(("Encountered an issue while determining "
			      "reference array.\n"));
	}

	ck_assert_int_eq(test_len,ref_len);

	switch(prec){
	case float_precision:
		compareArrayEntries_float(
			(const float*)ref, (const float*)test, ref_len,
			(float)tol, rel, (float)abs_zero_tol);
		break;
	case double_precision:
		compareArrayEntries_double(
			(const double*)ref, (const double*)test, ref_len,
			tol, rel, abs_zero_tol);
		break;
	}
	free(ref);
}
