#include <stdlib.h>
#include <stdio.h>
#include <check.h>

#include "../testtools/testtools.h"

#include "../../src/transient/rollSigma.h"
#include "../../src/utils.h" // IsLittleEndian

START_TEST (check_roll_sigma)
{
	float* original_input = NULL;

	int input_length = readDoubleArray(
		"tests/test_files/roll_sigma/roll_sigma_sample_input",
		IsLittleEndian(), float_precision, (void **)&original_input);
	ck_assert_int_eq(input_length, 51);

	float sigma_buffer[10];
	int numWindows = 10;
	int startIndex = 0;
	int interval = 5;
	int sigWindowSize = 13;
	float scaleFactor = 2.0;

	rollSigma(startIndex, interval, scaleFactor, sigWindowSize,
		  input_length, numWindows, original_input, sigma_buffer);
	free(original_input);

	compareAgainstSavedArray(
		"tests/test_files/roll_sigma/roll_sigma_sample_test",
		sigma_buffer, 10, float_precision, 1.e-5, 1, 1.e-5);
}
END_TEST

// We could extend the following test by:
//    - calculating sigma more than once (this allows us to test how the number
//      of samples in the window changes as you approach the end of the input)
//    - I could add additive Gaussian noise with increasing levels and check
//      that the sigma increases as we expect
START_TEST (check_rollSigma_uniform_waves)
{
	// this test is defined to be extremely simple
	// We are just going to check that a single value computed for an
	// entire sine wave is what we would expect

	// Recall, rollSigma returns scaleFactor*std/powf(nobs,0.2) at each
	// interval. Note, nobs is the number of samples in the window.
	// We define the problem such that
	//    - only 1 value of scaleFactor is computed
	//    - nobs = 1024
	//    - scaleFactor = 1
	// The output array should contain just a single value equal 0.25*std
	// The standard deviation of a Sine Wave is Amplitude/sqrt(2). Thus,
	// if our input Sine Wave has amplitude 1, then the output should
	// simply be `0.25/sqrt(2)` 

	// using a longer wave will decrease the error.
	const int length = 1024;
	
	float input[length];
	float sigma_buffer[1] = {0.f};
	int numWindows = 1;
	int sigWindowSize = length;
	int startIndex = length/2;
	int interval = 5;
	float scaleFactor = 1.0;

	{
		genSineWave((void *)input, float_precision, length, 1., 256,
			    1., 0.);
		rollSigma(startIndex, interval, scaleFactor, sigWindowSize,
			  length, numWindows, input, sigma_buffer);
		float expected = 0.25/sqrt(2.);
		float rel_err = (sigma_buffer[0]-expected)/expected;
		//printf("rel_error:%e\n",rel_err);
		ck_assert(fabsf(rel_err)<1.5e-3); // this was calibrated
	}


	// Now we can repeat the exercise with a square wave. In this case,
	// the standard deviation is just the amplitude
	{
		// I'm going to nudge the phase of the wave so that we never
		// sample exactly on the discontinuity
		double phase = M_PI/3.;
		genSquareWave((void *)input, float_precision, length, 1., 256,
			      1., phase);
		rollSigma(startIndex, interval, scaleFactor, sigWindowSize,
			  length, numWindows, input, sigma_buffer);
		float expected = 0.25;
		float rel_err = (sigma_buffer[0]-expected)/expected;
		//printf("rel_error:%e\n",rel_err);
		ck_assert(fabsf(rel_err)<7.5e-4); // this was calibrated
	}
}
END_TEST

Suite *rollSigma_suite()
{
	Suite *s = suite_create("rollSigma");
	TCase *tc_rollSigma = tcase_create("main");
	tcase_add_test(tc_rollSigma, check_rollSigma_uniform_waves);

	if (IsLittleEndian() == 1){
		tcase_add_test(tc_rollSigma, check_roll_sigma);
	} else {
		printf("Cannot add rollSigma tests because the tests are not \n"
		       "presently equipped to run on Big Endian machines\n");
	}
	suite_add_tcase(s, tc_rollSigma);
	return s;
}
