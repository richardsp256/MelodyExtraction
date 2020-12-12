#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <check.h>
#include "../src/onset/gammatoneFilter.h"
#include "doubleArrayTesting.h"
#include "../src/utils.h" // IsLittleEndian


void stepFunction(double **array,int length, double start, double increment,
		  int steplength){
	double cur = start;
	int i=0;
	int j=0;
	while (i<length){
		if (i < j*steplength){
			(*array)[i] = cur;
		} else {
			j++;
			cur = increment*((double)j)+start;
			(*array)[i] = cur;
		}
		i++;
	}
}

void testSOSCoefFramework(float centralFreq, int samplerate, double *ref,
			      double tol, int rel, double abs_zero_tol){
	double *coef = malloc(sizeof(double)*24);
	sosCoef(centralFreq, samplerate, coef);
	compareArrayEntries(ref, coef, 24, tol, rel, abs_zero_tol);
	free(coef);
}

void testSOSGammatoneFramework(float centralFreq, int samplerate,
				   double *input, int length, double *ref,
				   double tol, int rel, double abs_zero_tol){

	double *result = malloc(sizeof(double)*length);
	sosGammatoneHelper(input, result, centralFreq, samplerate, length);
	compareArrayEntries(ref, result, length, tol, rel, abs_zero_tol);
	free(result);
}

START_TEST(test_sos_coefficients_1)
{
	double ref[] = {6.1031107e-02, -1.2118071e-01, 0., 1., -1.5590685e+00,
			8.5722460e-01, 5.5986645e-02, 2.3877628e-02, 0., 1.,
			-1.5590685e+00, 8.5722460e-01, 1.3816354e-01,
			-1.3629211e-01, 0., 1., -1.5590685e+00, 8.5722460e-01,
			1.2797372e-01, -7.3279486e-02, 0., 1., -1.5590685e+00,
			8.5722460e-01};
	int rel = 1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;

	testSOSCoefFramework(1000., 11025, ref,
			     tol, rel, abs_zero_tol);
}
END_TEST

START_TEST(test_sos_coefficients_2)
{
	double ref1[] = {9.1368911e-02, -2.0813450e-01, 0., 1., -9.8759824e-01,
			 7.8999199e-01, 8.6315676e-02, 1.1137823e-01, 0., 1.,
			 -9.8759824e-01, 7.8999199e-01, 2.0164386e-01,
			 -1.6129740e-01, 0., 1., -9.8759824e-01, 7.8999199e-01,
			 1.9219808e-01, -3.6072876e-02, 0., 1., -9.8759824e-01,
			 7.8999199e-01};

	int rel = 1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;

	testSOSCoefFramework(2500., 16000, ref1,
			     tol, rel, abs_zero_tol);
}
END_TEST

START_TEST(test_sos_coefficients_3)
{
	double ref2[] = {2.5925251e-02, -3.0544150e-02, 0., 1., -1.9335553e+00,
			 9.4232544e-01, 2.0521109e-02, -1.5501504e-02, 0., 1.,
			 -1.9335553e+00, 9.4232544e-01, 5.8263388e-02,
			 -5.8440828e-02, 0., 1., -1.9335553e+00, 9.4232544e-01,
			 4.7312338e-02, -4.4024593e-02, 0., 1., -1.9335553e+00,
			 9.4232544e-01};
	int rel = 1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;

	testSOSCoefFramework(115., 8000, ref2,
			     tol, rel, abs_zero_tol);
}
END_TEST


START_TEST(test_sos_performance_1)
{
	double *impulse_input = calloc(1000,sizeof(double));
	impulse_input[0] = 1.;
	int length;
	double *ref1;
	length = readDoubleArray(("tests/test_files/gammatone/"
				  "sos_gammatone_response1"),
				 IsLittleEndian(), &ref1);

	int rel = 1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;

	testSOSGammatoneFramework(1000, 11025, impulse_input, 100,
				  ref1, tol, rel, abs_zero_tol);

	free(impulse_input);
}
END_TEST

START_TEST(test_sos_performance_2)
{
	double *input2 = calloc(100,sizeof(double));
	stepFunction(&input2,100, -2.0, 3.0,17);

	int length;
	double *ref2;
	length = readDoubleArray(("tests/test_files/gammatone/"
				  "sos_gammatone_response2"),
				 IsLittleEndian(), &ref2);

	int rel = 1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;

	testSOSGammatoneFramework(115, 8000, input2, 100,
				  ref2, tol, rel, abs_zero_tol);
	free(input2);
}
END_TEST


int buildInputArray(int *intInput, int intInputLen, int intInputStart,
		    double *dblInput, int dblInputLen, int dblInputStart,
		    char *strInput, double **inputArray){
	/* this is where we actually build the input array. */
	int length;
	if (strcmp(strInput,"delta") == 0){
		ck_assert_int_eq(intInputLen-intInputStart,1);
		ck_assert_int_eq(dblInputLen-dblInputStart,1);

		double amplitude = dblInput[dblInputStart];
		length = intInput[intInputStart];

		(*inputArray) = malloc(sizeof(double) * length);
		(*inputArray)[0] = amplitude;
		for (int i = 1; i < length; i++){
			(*inputArray)[i] = 0;
		}

	} else if (strcmp(strInput,"step") == 0) {
		ck_assert_int_eq(intInputLen - intInputStart, 2);
		ck_assert_int_eq(dblInputLen - dblInputStart, 2);


		length = intInput[intInputStart];
		int steplength = intInput[intInputStart+1];

		double start = dblInput[dblInputStart];
		double increment = dblInput[dblInputStart+1];

		(*inputArray) = malloc(sizeof(double) * length);
		//stepFunction(double *array,int length, double start,
		//	     double increment, int steplength)
		stepFunction(inputArray,length, start, increment,
			     steplength);
	} else {
		// we assume that the string is the name of a file
		ck_assert_int_eq(intInputLen-intInputStart,0);
		ck_assert_int_eq(dblInputLen-dblInputStart,0);
		int littleEndian = IsLittleEndian();
		length = readDoubleArray(strInput, littleEndian, inputArray);
	}
	return length;
}

int biquadFilterTestTemplate(int *intInput, int intInputLen, double *dblInput,
			     int dblInputLen, char *strInput, double **array)
{
	/* This is a template for testing the biquadFilter or cascadeBiquad. 
	 * 
	 * We always expect the first 6 doubles in dblInput to be the 
	 * coefficients.
	 * We also always expect the first int in intInput to indicate the  
	 * number of biquad sections. A value of zero indicates that the normal 
	 * biquadFilter Should be used
	 * Additional arguments are slightly dependent on strInput. 
	 *
	 * Valid strInput values include:
	 *  -the name of a file. In this case the file is assumed contain 
	 *   the array to be filtered. intInput should contain no values and 
	 *   dblInput should only contain the coefficients.
	 *  -"delta" - the input array is a delta function.
	 *  -"step" - the input array is a step function. 
	 */

	
	ck_assert (intInputLen>=1);
	int num_stages = intInput[0];
	ck_assert (num_stages>=0);
	
	/* First we check to see how many coefficients should be specified. */
	int num_coef = 6;
	if (num_stages > 0){
		num_coef *= intInput[0];
	}

	ck_assert (dblInputLen>=num_coef);

	double *inputArray;
	int length;

	length = buildInputArray(intInput, intInputLen, 1, dblInput,
				 dblInputLen, num_coef, strInput, &inputArray);
	(*array) = malloc(sizeof(double) * length);
	if (num_stages == 0){
		biquadFilter(dblInput, inputArray, *array, length);
	} else {
		cascadeBiquad(num_stages, dblInput, inputArray, *array, length);
	}
	free(inputArray);
	return length;
}

void destroy_test_table(int *length, struct dblArrayTestEntry **table)
{
	if (*length!=0){
		for (int i =0; i<*length; i++){
			clean_up_dblArrayTestEntry((*table) + i);
		}
		free(*table);
		*length = 0;
	}
}

static struct dblArrayTestEntry *biquad_filter_table;
static int biquad_table_length=0;


struct dblArrayTestEntry* construct_biquad_test_table()
{
	/* I am unsure of whether or not an unchecked fixture is called before 
	 * every loop in a loop test, or if it's called once at the start. 
	 * If the former case is true, then this is better off as an unchecked
	 * test fixture.
	 */
	biquad_table_length = 5;
	biquad_filter_table = malloc(sizeof(struct dblArrayTestEntry)
				     * biquad_table_length);

	/* Setup the simplest test.
	 * We are literally just running a biquad filter with the value the 
	 * only non-zero coefficient value of 13.
	 *
	 * We could honestly package this test differently because we just 
	 * expect the result to be an array of 100 values where all values are 
	 * 0, except for the very first value which has a value of 13
	 */

	int intInput[] = {0,100};
	// the first 6 values correspond to the biquad coefficients
	double dblInput[] = {13.,0.,0.,1.,0.,0.,
			     1.};
	
	setup_dblArrayTestEntry((biquad_filter_table+0),intInput, 2,
				dblInput, 7, "delta",
				("tests/test_files/gammatone/"
				 "simple_biquad_test_result"),
				&biquadFilterTestTemplate);

	/* Setup the test with all non-zero coeficients in the numerator of the 
	 * transfer function. Again, the output is very simple and could be 
	 * easily formatted differently */

	double dblInput2[] = {13.,-4.,7.,1.,0.,0.,
			      1.};
	setup_dblArrayTestEntry((biquad_filter_table+1),intInput, 2,
				dblInput2, 7, "delta",
				("tests/test_files/gammatone/"
				 "simple_biquad_numerator_result"),
				&biquadFilterTestTemplate);

	/* Setup the test with all non-zero coeficients in the denominator of 
	 * the transfer function. 
	 */

	double dblInput3[] = {1.,0.,0.,1.,6.,-5.,
			      1.};
	setup_dblArrayTestEntry((biquad_filter_table+2),intInput, 2,
				dblInput3, 7, "delta",
				("tests/test_files/gammatone/"
				 "simple_biquad_denominator_result"),
				&biquadFilterTestTemplate);

	/* Setup the test with all non-zero coeficients in the transfer 
	 * function.
	 */
	
	double dblInput4[] = {13.,-4.,7.,1.,6.,-5.,
			      1.};
	setup_dblArrayTestEntry((biquad_filter_table+3),intInput, 2,
				dblInput4, 7, "delta",
				("tests/test_files/gammatone/"
				 "simple_biquad_all_coef_result"),
				&biquadFilterTestTemplate);

	/* This last sample is slightly more complex. It tests the biquad 
	 * filter on an input step function. */

	double dblInput5[] = {13.,-4.,7.,1.,6.,-5.,
			      3.,18.};
	int intInput5[] = {0,10,2};
	setup_dblArrayTestEntry((biquad_filter_table+4),intInput5, 3,
				dblInput5, 8, "step",
				("tests/test_files/gammatone/"
				 "step_function_biquad_response"),
				&biquadFilterTestTemplate);
}


START_TEST (check_biquad_filter_table)
{
	ck_assert (process_double_array_test(biquad_filter_table[_i],
					     1.e-5, 1, 1.e-5));
}
END_TEST


static struct dblArrayTestEntry *cascade_biquad_filter_table;
static int cascade_biquad_table_length=0;


struct dblArrayTestEntry* construct_cascade_biquad_test_table()
{
	cascade_biquad_table_length = 1;
	cascade_biquad_filter_table = malloc(sizeof(struct dblArrayTestEntry)
					     * cascade_biquad_table_length);

	/* The first tests is a simple rehash of a biquadFilter test 
	 * test. It tests that cascadeBiquad properly handles 1 order.
	 */
	double dblInput[] = {13.,-4.,7.,1.,6.,-5.,
			      3.,18.};
	int intInput[] = {1,10,2};
	setup_dblArrayTestEntry((cascade_biquad_filter_table+0),intInput, 3,
				dblInput, 8, "step",
				("tests/test_files/gammatone/"
				 "step_function_biquad_response"),
				&biquadFilterTestTemplate);
}

START_TEST (check_cascade_biquad_filter_table)
{
	ck_assert (process_double_array_test(cascade_biquad_filter_table[_i],
					     1.e-5, 1, 1.e-5));
}
END_TEST

Suite *gammatone_suite()
{
	Suite *s = suite_create("Gammatone");
	TCase *tc_coef = tcase_create("Coef");
	TCase *tc_performance = tcase_create("Performance");
	TCase *tc_biquad = tcase_create("Biquad Filter");
	TCase *tc_cascadeBiquad = tcase_create("cascadeBiquad");

	tcase_add_test(tc_coef,test_sos_coefficients_1);
	tcase_add_test(tc_coef,test_sos_coefficients_2);
	tcase_add_test(tc_coef,test_sos_coefficients_3);

	tcase_add_test(tc_performance,test_sos_performance_1);
	tcase_add_test(tc_performance,test_sos_performance_2);

	if (IsLittleEndian() == 1){
		tcase_add_loop_test(tc_biquad,check_biquad_filter_table,0,
				    biquad_table_length);
		tcase_add_loop_test(tc_cascadeBiquad,
				    check_cascade_biquad_filter_table, 0,
				    cascade_biquad_table_length);
	} else {
		printf("Cannot add biquad filter tests or cascadeBiquad \n"
		       "tests because the tests are not presently equipped \n"
		       "to run on Big Endian machines.\n");
	}
	suite_add_tcase(s, tc_biquad);
	suite_add_tcase(s, tc_cascadeBiquad); 
	
	suite_add_tcase(s, tc_coef);
	suite_add_tcase(s, tc_performance);
	return s;
}

int main(void){
	construct_biquad_test_table();
	construct_cascade_biquad_test_table();
	Suite *s = gammatone_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	destroy_test_table(&biquad_table_length, &biquad_filter_table);
	destroy_test_table(&cascade_biquad_table_length,
	                   &cascade_biquad_filter_table);
	if (number_failed == 0){
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
