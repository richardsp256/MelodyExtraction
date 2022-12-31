#include <stdbool.h>

typedef int (*dblArrayTest)(int *intInput, int intInputLen, double *dblInput,
			    int dblInputLen, char *strInput, double **array);
#ifndef DBLARRAYTEST_H
#define DBLARRAYTEST_H

enum PrecisionEnum {
	float_precision,
	double_precision,
};
#endif /*DBLARRAYTEST_H*/

/// Reads in a little-endian floating point array from a binary file. The
/// entries in the file are assumed to be stored in double precision.
///
/// @param[in]  fileName Null-terminated string denoting the input file,
///     relative to the root directory of the repository
/// @param[in]  system_little_endian Bool indicating whether the host system is
///     little endian.
/// @param[in]  output_prec Indicates the precision of floating point entries
///     that should be held in the output array
/// @param[out] array This should be a pointer to a void pointer. The inner
///     pointer will be allocated off of the heap to hold the loaded data.
/// @returns Returns the array length on success. A negative value indicates
///     that there was an issue.
int readDoubleArray(const char *fileName, bool system_little_endian,
		    enum PrecisionEnum output_prec, void **array);

void compareArrayEntries_double(const double *ref, const double* other,
				int len, double tol, int rel,
				double abs_zero_tol);
void compareArrayEntries_float(const float *ref, const float* other, int len,
			       float tol, int rel, float abs_zero_tol);

void compareAgainstSavedArray(const char* fname, const void* test,
			      int test_len, enum PrecisionEnum prec,
			      double tol, bool rel, double abs_zero_tol);
