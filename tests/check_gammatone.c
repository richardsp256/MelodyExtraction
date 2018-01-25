#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <check.h>
#include "../src/gammatoneFilter.h"
#include "doubleArrayTesting.h"


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
	allPoleCoef(centralFreq, samplerate, coef);
	compareArrayEntries(ref, coef, 24, tol, rel, abs_zero_tol);
	free(coef);
}

void testSOSGammatoneFramework(float centralFreq, int samplerate,
				   double *input, int length, double *ref,
				   double tol, int rel, double abs_zero_tol){

	double *result = malloc(sizeof(double)*length);
	allPoleGammatoneHelper(input, &result, centralFreq, samplerate, length);
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

	double ref1[] = {6.7683936e-17, 2.1104779e-16, 2.4239155e-16,
			 -1.4876671e-16, -1.1331346e-15, -2.4695411e-15,
			 -3.4440334e-15, -3.1343673e-15, -9.2002259e-16,
			 3.0133997e-15, 7.4745466e-15, 1.0557555e-14,
			 1.0381404e-14, 5.9970156e-15, -1.9466179e-15,
			 -1.1132757e-14, -1.8233237e-14, -2.0110857e-14,
			 -1.5126992e-14, -4.0193326e-15, 1.0060513e-14,
			 2.2454080e-14, 2.8581038e-14, 2.5646008e-14,
			 1.3823791e-14, -3.5618239e-15, -2.0997375e-14,
			 -3.2589734e-14, -3.4111317e-14, -2.4553863e-14,
			 -6.6166725e-15, 1.4079451e-14, 3.0812802e-14,
			 3.8005663e-14, 3.3124484e-14, 1.7576952e-14,
			 -3.7107607e-15, -2.3900910e-14, -3.6494625e-14,
			 -3.7467944e-14, -2.6578874e-14, -7.4166384e-15,
			 1.3808726e-14, 3.0317848e-14, 3.6963977e-14,
			 3.1859387e-14, 1.6907246e-14, -2.9066224e-15,
			 -2.1196912e-14, -3.2261313e-14, -3.2875479e-14,
			 -2.3231246e-14, -6.7477481e-15, 1.1139880e-14,
			 2.4784009e-14, 3.0117890e-14, 2.5866809e-14,
			 1.3818143e-14, -1.8902313e-15, -1.6188239e-14,
			 -2.4712622e-14, -2.5136122e-14, -1.7783601e-14,
			 -5.3805073e-15, 7.9372922e-15, 1.7997285e-14,
			 2.1892786e-14, 1.8809500e-14, 1.0146682e-14,
			 -1.0601169e-15, -1.1188766e-14, -1.7193376e-14,
			 -1.7509479e-14, -1.2436708e-14, -3.9158586e-15,
			 5.1876918e-15, 1.2036074e-14, 1.4693514e-14,
			 1.2657383e-14, 6.9063074e-15, -5.1340397e-16,
			 -7.1998351e-15, -1.1162608e-14, -1.1403387e-14,
			 -8.1447921e-15, -2.6647521e-15, 3.1810314e-15,
			 7.5759533e-15, 9.2970380e-15, 8.0412438e-15,
			 4.4418458e-15, -2.0430209e-16, -4.3903504e-15,
			 -6.8789815e-15, -7.0581776e-15, -5.0744952e-15,
			 -1.7221623e-15, 1.8566496e-15, 4.5517834e-15,
			 5.6215961e-15};


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

	double ref2[] = {2.4414063e-16, 1.1882594e-15, 3.4514247e-15,
		       7.7549994e-15, 1.4852864e-14, 2.5456417e-14,
		       4.0157853e-14, 5.9355054e-14, 8.3181170e-14,
		       1.1144163e-13, 1.4356102e-13, 1.7854179e-13,
		       2.1493645e-13, 2.5083429e-13, 2.8386344e-13,
		       3.1120843e-13, 3.2964319e-13, 3.3631121e-13,
		       3.2868989e-13, 3.0451932e-13, 2.6170466e-13,
		       1.9820115e-13, 1.1189041e-13, 4.5659077e-16,
		       -1.3873032e-13, -3.0871686e-13, -5.1304851e-13,
		       -7.5582707e-13, -1.0417395e-12, -1.3760553e-12,
		       -1.7645912e-12, -2.2136427e-12, -2.7298833e-12,
		       -3.3202329e-12, -3.9909660e-12, -4.7476247e-12,
		       -5.5949606e-12, -6.5368976e-12, -7.5765119e-12,
		       -8.7160225e-12, -9.9567894e-12, -1.1299313e-11,
		       -1.2743233e-11, -1.4287324e-11, -1.5929484e-11,
		       -1.7666719e-11, -1.9495116e-11, -2.1409816e-11,
		       -2.3404968e-11, -2.5473697e-11, -2.7608049e-11,
		       -2.9798222e-11, -3.2032611e-11, -3.4297925e-11,
		       -3.6579333e-11, -3.8860657e-11, -4.1124595e-11,
		       -4.3352960e-11, -4.5526942e-11, -4.7627368e-11,
		       -4.9634971e-11, -5.1530652e-11, -5.3295727e-11,
		       -5.4912167e-11, -5.6362813e-11, -5.7631573e-11,
		       -5.8703596e-11, -5.9565416e-11, -6.0204341e-11,
		       -6.0608649e-11, -6.0767797e-11, -6.0672650e-11,
		       -6.0315712e-11, -5.9691347e-11, -5.8795990e-11,
		       -5.7628334e-11, -5.6189486e-11, -5.4483092e-11,
		       -5.2515422e-11, -5.0295408e-11, -4.7834644e-11,
		       -4.5147333e-11, -4.2250192e-11, -3.9162308e-11,
		       -3.5904956e-11, -3.2500639e-11, -2.8972922e-11,
		       -2.5346287e-11, -2.1645992e-11, -1.7897941e-11,
		       -1.4128551e-11, -1.0364613e-11, -6.6331555e-12,
		       -2.9612847e-12, 6.2397815e-13, 4.0958775e-12,
		       7.4281174e-12, 1.0595070e-11, 1.3571994e-11,
		       1.6335269e-11};

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
		int littleEndian = isLittleEndian();
		length = readDoubleArray(strInput, littleEndian, inputArray);
	}
	return length;
}

int biquadFilterTestTemplate(int *intInput, int intInputLen, double *dblInput,
			     int dblInputLen, char *strInput, double **array)
{
	/* This is a template for testing the biquadFilter. 
	 * 
	 * We always expect the first 6 doubles in dblInput to be the 
	 * coefficients.
	 * Things are slightly dependent on strInput. 
	 *
	 * Valid strInput values include:
	 *  -the name of a file. In this case the file is assumed contain 
	 *   the array to be filtered. intInput should contain no values and 
	 *   dblInput should only contain the coefficients.
	 *
	 * Below are some other potential strInput values I'm considering.
	 *  -"delta" - the input array is a delta function.
	 *  -"step" - the input array is a step function. 
	 */

	ck_assert (dblInputLen>=6);
	
	double *inputArray;
	int length;

	length = buildInputArray(intInput, intInputLen, 0, dblInput,
				 dblInputLen, 6, strInput, &inputArray);
	(*array) = malloc(sizeof(double) * length);
	biquadFilter(dblInput, inputArray, *array, length);
	free(inputArray);
	return length;
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

	int intInput[] = {100};
	// the first 6 values correspond to the biquad coefficients
	double dblInput[] = {13.,0.,0.,1.,0.,0.,
			     1.};
	
	setup_dblArrayTestEntry((biquad_filter_table+0),intInput, 1,
				dblInput, 7, "delta",
				("tests/test_files/gammatone/"
				 "simple_biquad_test_result"),
				&biquadFilterTestTemplate);

	/* Setup the test with all non-zero coeficients in the numerator of the 
	 * transfer function. Again, the output is very simple and could be 
	 * easily formatted differently */

	double dblInput2[] = {13.,-4.,7.,1.,0.,0.,
			      1.};
	setup_dblArrayTestEntry((biquad_filter_table+1),intInput, 1,
				dblInput2, 7, "delta",
				("tests/test_files/gammatone/"
				 "simple_biquad_numerator_result"),
				&biquadFilterTestTemplate);

	/* Setup the test with all non-zero coeficients in the denominator of 
	 * the transfer function. 
	 */

	double dblInput3[] = {1.,0.,0.,1.,6.,-5.,
			      1.};
	setup_dblArrayTestEntry((biquad_filter_table+2),intInput, 1,
				dblInput3, 7, "delta",
				("tests/test_files/gammatone/"
				 "simple_biquad_denominator_result"),
				&biquadFilterTestTemplate);

	/* Setup the test with all non-zero coeficients in the transfer 
	 * function.
	 */
	
	double dblInput4[] = {13.,-4.,7.,1.,6.,-5.,
			      1.};
	setup_dblArrayTestEntry((biquad_filter_table+3),intInput, 1,
				dblInput4, 7, "delta",
				("tests/test_files/gammatone/"
				 "simple_biquad_all_coef_result"),
				&biquadFilterTestTemplate);

	double dblInput5[] = {13.,-4.,7.,1.,6.,-5.,
			      3.,18.};
	int intInput5[] = {10,2};
	setup_dblArrayTestEntry((biquad_filter_table+4),intInput5, 2,
				dblInput5, 8, "step",
				("tests/test_files/gammatone/"
				 "step_function_biquad_response"),
				&biquadFilterTestTemplate);

}

void destroy_biquad_test_table(){
	if (biquad_table_length!=0){
		for (int i =0; i<biquad_table_length; i++){
			clean_up_dblArrayTestEntry(biquad_filter_table + i);
		}
		free(biquad_filter_table);
		biquad_table_length = 0;
	}
}

START_TEST (check_biquad_filter_table)
{
	ck_assert (process_double_array_test(biquad_filter_table[_i],
					     1.e-5, 1, 1.e-5));
}
END_TEST


Suite *gammatone_suite()
{
	Suite *s = suite_create("Gammatone");
	TCase *tc_coef = tcase_create("Coef");
	TCase *tc_performance = tcase_create("Performance");
	TCase *tc_biquad = tcase_create("Biquad Filter");

	tcase_add_test(tc_coef,test_sos_coefficients_1);
	tcase_add_test(tc_coef,test_sos_coefficients_2);
	tcase_add_test(tc_coef,test_sos_coefficients_3);

	tcase_add_test(tc_performance,test_sos_performance_1);
	tcase_add_test(tc_performance,test_sos_performance_2);

	if (isLittleEndian() == 1){
		tcase_add_loop_test(tc_biquad,check_biquad_filter_table,0,
				    biquad_table_length);
	} else {
		printf("Cannot add biquad filter tests because the tests are \n"
		       "not presently equipped to run on Big Endian machines"
		       "\n");
	}
	suite_add_tcase(s, tc_biquad);
	
	suite_add_tcase(s, tc_coef);
	suite_add_tcase(s, tc_performance);
	return s;
}

int main(void){
	construct_biquad_test_table();
	Suite *s = gammatone_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	destroy_biquad_test_table();
	if (number_failed == 0){
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
