#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <check.h>
#include <float.h> // FLT_MAX

// include function declarations for unit testing helpers
#include "../testtools/testtools.h"

// includes from melodyextraction library
#include "../../src/transient/gammatoneFilter.h"
#include "../../src/utils.h" // IsLittleEndian
#include "../../src/errors.h"

// define tests of the sosGammatone coefficient generation function
//============================================================================

void testSOSCoefFramework(float centralFreq, int samplerate, double *ref,
			      double tol, int rel, double abs_zero_tol){
	double *coef = malloc(sizeof(double)*24);
	sosGammatoneCoef(centralFreq, samplerate, coef);
	compareArrayEntries_double(ref, coef, 24, tol, rel, abs_zero_tol);
	free(coef);
}

START_TEST(test_sosGammatone_coef_1)
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

START_TEST(test_sosGammatone_coef_2)
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

START_TEST(test_sosGammatone_coef_3)
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

// define full tests of the sosGammatone
//============================================================================

void testSOSGammatoneFramework(float centralFreq, int samplerate,
			       float *input, int length,
			       const char * ref_fname, double tol, int rel,
			       double abs_zero_tol){

       float *result = malloc(sizeof(float)*length);
       sosGammatone(input, result, centralFreq, samplerate, length, NULL);
       compareAgainstSavedArray(ref_fname, result, length, float_precision,
				tol, rel, abs_zero_tol);
       free(result);
}

START_TEST(test_sosGammatone_Impulse)
{
	float impulse_input[100] = {0.}; // initialize to zeros
	genImpulse((void *)(&impulse_input[0]), float_precision, 100, 1.);

	int rel = 1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;

	testSOSGammatoneFramework(
		1000, 11025, impulse_input, 100,
		"tests/test_files/gammatone/sos_gammatone_impulse_response",
		tol, rel, abs_zero_tol);
}
END_TEST

START_TEST(test_sosGammatone_Staircase)
{
	float staircase_input[100];
	genStaircaseFunction((void*) staircase_input, float_precision, 100,
			     -2.0, 3.0, 17);

	int rel = 1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;
	
	testSOSGammatoneFramework(
		115, 8000, staircase_input, 100,
		"tests/test_files/gammatone/sos_gammatone_staircase_response",
		tol, rel, abs_zero_tol);
}
END_TEST

// the gain at the central frequency should be zero
// in other words, the amplitude should be unchanged
START_TEST(test_sosGammatone_centralFreq_gain)
{
	float input[1000] = {0.}; // initialize to zeros

	double period = 1./100; // units of seconds
	size_t sample_rate = 1000; // units of Hz
	double amplitude = 1.;
	double phase = 0.;
	genSineWave((void *)input, float_precision, 1000, period, sample_rate,
		    amplitude, phase);

	float output[1000];
	sosGammatone(input, output, 1./period, (int)sample_rate, 1000, NULL);

	// There may be a more robust way to measure the gain
	float cur_max = -FLT_MAX;
	float cur_min = FLT_MAX;
	// start a little off from the start since the first couple of samples
	// might be wrong
	for (int i = 20; i < 1000; i++){
		cur_max = fmaxf(cur_max, output[i]);
		cur_min = fminf(cur_min, output[i]);
	}
	//printf("%e\n%e\n", cur_max - 1., cur_min+1);
	// this may be unacceptable
	ck_assert(fabsf(cur_max - 1.f)<0.1); // this was calibrated
	ck_assert(fabsf(cur_min + 1.f)<0.1); // this was calibrated
}
END_TEST

// test the sosFilter's error handling
//============================================================================
// I'm not actually going to rigorously test the precise return code (since
// those may change

START_TEST (check_sosFilter_ArrayOverlap)
{
	double coef[6] = {13.,0.,0.,1.,0.,0.};
	float input[20] = {0.};
	genImpulse((void *)(&input[0]), float_precision, 20, 1.);

	// check that when the input and output array overlap, that the
	// appropriate return value is provided
	int rv;
	rv = sosFilter(1, coef, input, input, 10, NULL);
	ck_assert_int_ne(rv, ME_SUCCESS);

	rv = sosFilter(1, coef, input, &(input[0])+5, 10, NULL);
	ck_assert_int_ne(rv, ME_SUCCESS);

	rv = sosFilter(1, coef, &(input[0])+5, input, 10, NULL);
	ck_assert_int_ne(rv, ME_SUCCESS);

	// this one should be succesful
	rv = sosFilter(1, coef, input, &(input[0])+10, 10, NULL);
	ck_assert_int_eq(rv, ME_SUCCESS);
}

START_TEST (check_sosFilter_zero_stage)
{
	double coef[6] = {13.,0.,0.,1.,0.,0.};
	float input[10];
	genImpulse((void *)(&input[0]), float_precision, 10, 1.);
	float output[10];

	// check that when the input and output array overlap, that the
	// appropriate return value is provided
	int rv;
	rv = sosFilter(0, coef, input, output, 10, NULL);
	ck_assert_int_ne(rv, ME_SUCCESS);

	// this one should be succesful
	rv = sosFilter(1, coef, input, output, 10, NULL);
	ck_assert_int_eq(rv, ME_SUCCESS);
}


// define biquad filter tests
//============================================================================

START_TEST (check_biquad_simple_impulse)
{
	/* Setup the simplest test.
	 * We are literally just running a biquad filter with the value the 
	 * only non-zero coefficient value of 13.
	 *
	 * We could honestly package this test differently because we just 
	 * expect the result to be an array of 100 values where all values are 
	 * 0, except for the very first value which has a value of 13
	 */
	double coef[6] = {13.,0.,0.,1.,0.,0.};
	float input[100];
	genImpulse((void *)(&input[0]), float_precision, 100, 1.);
	float output[100];
	sosFilter(1, coef, input, output, 100, NULL);
	compareAgainstSavedArray(
		"tests/test_files/gammatone/simple_biquad_test_result",
		output, 100, float_precision, 1.e-5, 1, 1.e-5
		);
}
END_TEST

START_TEST (check_biquad_simple_numerator_impulse)
{
	// Setup the test with all non-zero coeficients in the numerator of the 
	// transfer function. Again, the output is very simple and could be 
	// easily formatted differently */
	double coef[6] = {13.,-4.,7.,1.,0.,0.};
	float input[100];
	genImpulse((void *)(&input[0]), float_precision, 100, 1.);
	float output[100];
	sosFilter(1, coef, input, output, 100, NULL);
	compareAgainstSavedArray(
		"tests/test_files/gammatone/simple_biquad_numerator_result",
		output, 100, float_precision, 1.e-5, 1, 1.e-5
		);
}
END_TEST



// The following 2 tests need to be updated so that the expected result fits
// into a float (currently they exceed FLT_MAX
//START_TEST (check_biquad_simple_denom_impulse)
//{
//	// Setup the test with all non-zero coeficients in the denominator of 
//	// the transfer function. 
//	double coef[6] = {1.,0.,0.,1.,6.,-5.};
//	double input[100];
//	genImpulse((void *)(&input[0]), double_precision, 100, 1.);
//	double output[100];
//	sosFilter(1, coef, input, output, 100, NULL);
//	compareAgainstSavedArray(
//		"tests/test_files/gammatone/simple_biquad_denominator_result",
//		output, 100, double_precision, 1.e-5, 1, 1.e-5
//		);
//}
//END_TEST
//START_TEST (check_biquad_all_coef_impulse)
//{
//	// Setup the test with all non-zero coeficients in the numerator of the 
//	// transfer function. Again, the output is very simple and could be 
//	// easily formatted differently
//	double coef[6] = {13.,-4.,7.,1.,6.,-5.};
//	double input[100];
//	genImpulse((void *)(&input[0]), double_precision, 100, 1.);
//	double output[100];
//	sosFilter(1, coef, input, output, 100, NULL);
//	compareAgainstSavedArray(
//		"tests/test_files/gammatone/simple_biquad_all_coef_result",
//		output, 100, double_precision, 1.e-5, 1, 1.e-5
//		);
//}
//END_TEST

START_TEST (check_biquad_all_coef_staircase)
{
	double coef[6] = {13.,-4.,7.,1.,6.,-5.};
	float input[10];
	genStaircaseFunction((void *)(&input[0]), float_precision, 10,
			     3., 18, 2);
	float output[10];
	sosFilter(1, coef, input, output, 10, NULL);
	compareAgainstSavedArray(
		"tests/test_files/gammatone/step_function_biquad_response",
		output, 10, float_precision, 1.e-5, 1, 1.e-5
		);
}
END_TEST

// Testing that the state variable properly allows chunking
START_TEST (check_biquad_all_coef_staircase_chunks)
{
	double coef[6] = {13.,-4.,7.,1.,6.,-5.};
	float input[10];
	genStaircaseFunction((void *)(&input[0]), float_precision, 10,
			     3., 18, 2);
	float output_ref[10];
	sosFilter(1, coef, input, output_ref, 10, NULL);


	double state[2] = {0.,0.};
	float output_chunks[10];
	sosFilter(1, coef, input, output_chunks, 4, state);
	sosFilter(1, coef, input+4, output_chunks+4, 6, state);

	compareArrayEntries_float(output_ref, output_chunks, 10,
				  0., 0, 0.);
}
END_TEST

Suite *gammatone_suite()
{
	Suite *s = suite_create("Gammatone");
	TCase *tc_coef = tcase_create("GammatoneCoef");
	TCase *tc_performance = tcase_create("Performance");
	TCase *tc_sosFilter_errs = tcase_create("sosFilter Error Handling");
	TCase *tc_biquad = tcase_create("Biquad Filter");

	tcase_add_test(tc_coef,test_sosGammatone_coef_1);
	tcase_add_test(tc_coef,test_sosGammatone_coef_2);
	tcase_add_test(tc_coef,test_sosGammatone_coef_3);

	tcase_add_test(tc_performance,test_sosGammatone_Impulse);
	tcase_add_test(tc_performance,test_sosGammatone_Staircase);
	tcase_add_test(tc_performance,test_sosGammatone_centralFreq_gain);

	tcase_add_test(tc_sosFilter_errs, check_sosFilter_ArrayOverlap);
	tcase_add_test(tc_sosFilter_errs, check_sosFilter_zero_stage);

	if (IsLittleEndian() == 1){
		tcase_add_test(tc_biquad, check_biquad_simple_impulse);
		tcase_add_test(tc_biquad,
			       check_biquad_simple_numerator_impulse);
		tcase_add_test(tc_biquad, check_biquad_all_coef_staircase);
	} else {
		printf("Cannot add biquad filter tests or sosFilter \n"
		       "tests because the tests are not presently equipped \n"
		       "to run on Big Endian machines.\n");
	}
	tcase_add_test(tc_biquad,check_biquad_all_coef_staircase_chunks);

	suite_add_tcase(s, tc_coef);
	suite_add_tcase(s, tc_performance);
	suite_add_tcase(s, tc_biquad);
	suite_add_tcase(s, tc_sosFilter_errs);

	return s;
}
