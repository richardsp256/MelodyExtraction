typedef int (*dblArrayTest)(int *intInput, int intInputLen, double *dblInput,
			    int dblInputLen, char *strInput, double **array);

struct dblArrayTestEntry{
	int *intInput;
	int intInputLen;
	double *dblInput;
	int dblInputLen;
	char *strInput;
	char *resultFname;
	dblArrayTest func;
};

int readDoubleArray(char *fileName, int system_little_endian, double *array);

void compareArrayEntries(double *ref, double* other, int length,
			 double tol, int rel,double abs_zero_tol);

void process_double_array_test(struct dblArrayTestEntry entry, double tol,
			       int rel, double abs_zero_tol);
