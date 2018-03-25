typedef int (*dblArrayTest)(int *intInput, int intInputLen, double *dblInput,
			    int dblInputLen, char *strInput, double **array);
#ifndef DBLARRAYTEST_H
#define DBLARRAYTEST_H
struct dblArrayTestEntry{
	int *intInput;
	int intInputLen;
	double *dblInput;
	int dblInputLen;
	char *strInput;
	char *resultFname;
	dblArrayTest func;
};
#endif /*DBLARRAYTEST_H*/

int isLittleEndian();

int readDoubleArray(char *fileName, int system_little_endian, double **array);

void compareArrayEntries(double *ref, double* other, int length,
			 double tol, int rel,double abs_zero_tol);

void setup_dblArrayTestEntry(struct dblArrayTestEntry *test_entry,
			     int intInput[], int intInputLen,
			     double dblInput[], int dblInputLen,
			     char *strInput, char *resultFname,
			     dblArrayTest func);
void clean_up_dblArrayTestEntry(struct dblArrayTestEntry *test_entry);

int process_double_array_test(struct dblArrayTestEntry entry, double tol,
			      int rel, double abs_zero_tol);

void float_to_double_array(float* array, int length, double** dblarray);
void double_to_float_array(double* array, int length, float** fltarray);
