#include <stdlib.h> // size_t
#include <stdio.h>
#include <stdalign.h>
#include <math.h> // fabs, expf, M_PI
#include <check.h>


#include "../testtools/testtools.h"

#include "../../src/transient/calcSummedLagCorrentrograms.h"
#include "../../src/errors.h"

// To help test our implementation of calcSummedLagCorrentrograms, we provide
// a straightforward, naive reference implementation

#define MAX_WINSIZE_OR_LAG 20

/// naive implementation for directly computing a correntrogram
/// @param[in]  x The input signal. This must hold `winsize+max_lag` entries
/// @param[in]  bandwisth The kernel bandwidth
/// @param[in]  winsize The length of the integration window
/// @param[in]  max_lag The maximum lag. The must exceed 0.
/// @param[out] out array where the correntrogram will be written. This must
///    have max_lag entries. The ith element in this array will hold the
///    correntropy with a lag of i+1.
void calc_correntrogram(const float * x, float bandwidth, size_t winsize,
			size_t max_lag, float * out)
{
	if (bandwidth <= 0){ abort(); }
	// For simplicity we use temporary stack allocated buffers.
	if (winsize > MAX_WINSIZE_OR_LAG || max_lag > MAX_WINSIZE_OR_LAG){
		printf("The winsize is too big!\n");
		abort();
	}

	for (size_t lag = 1; lag <= max_lag; lag++){
		// To play nicely with EvaluateKernel, we use an intermediate
		// buffer to pass values
		float input_buffer[MAX_WINSIZE_OR_LAG];
		float output_buffer[MAX_WINSIZE_OR_LAG];
		// fill input_buffer with the differences between the signal
		// and the lagged signal
		for (size_t i = 0; i < winsize; i++){
			input_buffer[i] = x[i] - x[i+lag];
		}
		// evaluate the kernel at each of these locations
		EvaluateKernel(input_buffer, output_buffer, winsize,
			       bandwidth);
		// now store the average kernel evaluation from the integration
		// window in out[lag-1]
		out[lag-1] = 0.;
		for (size_t i = 0; i <winsize; i++){
			out[lag-1] += output_buffer[i];
		}
		out[lag-1] /= ((float)winsize);
	}
}

/// the arguments are identical to those documented in calcSummedCorrentrograms
void NaiveSummedCorrentrograms(const float * x, const float * bandwidths,
			       size_t winsize, size_t max_lag, size_t hopsize,
			       size_t n_win, float * summed_acgrams)
{
	// For simplicity we use temporary stack allocated buffers.
	if (winsize > MAX_WINSIZE_OR_LAG || max_lag > MAX_WINSIZE_OR_LAG){
		printf("The winsize is too big!\n");
		abort();
	}
	for (size_t i = 0; i < n_win; i++){
		// Compute the correntrogram
		float correntrogram[MAX_WINSIZE_OR_LAG];
		calc_correntrogram(x+i*hopsize, bandwidths[i], winsize,
				   max_lag, (float*)correntrogram);
		// sum over the lags in the correntrogram and store the sum in
		// summed_acgrams
		summed_acgrams[i] = 0.;
		for (size_t lag = 1; lag <= max_lag; lag++){
			summed_acgrams[i] += correntrogram[lag-1];
		}
	}
}


// TODO: Add tests of functionallity. We could check answers if we perform
// calculations on the following input signals:
// 1. All zeros. The result will just be kernel(0)*winsize*max_lag
//    - this requires support for computing values from the kernel directly
//      (since it's an approximation of a Gaussian)
// 2. Square Wave
//    - this requires support for computing values from the kernel directly
// 3. Triangle Wave
//    - this requires support for computing values from the kernel directly
// 4. Sine Wave? This might be less robust. We can analyticaly derive a power
//    series that gives this result (assuming a continuous signal). But, this
//    has the following difficulties:
//    - we need to make sure that we expand the power series to include enough
//      terms
//    - we need to make sure that the sampling rate is fine enough (to
//      approximate a continuous signal)
//    - Additionally, the kernel is an approximation for a truncated Gaussian
// 5. Gaussian White noise. There is a specific expected value (if the kernel
//    bandwidth is at least double the standard deviation of the Gaussian).
//    Several of the same challenges that are listed above still apply.



// We should have an explicit test that checks that values get added to the
// correntrogram in-place

// tests the simple case: all entries in the input signal are a constant value
START_TEST(calcSummedLagCorrentrograms_uniform_signal)
{
	const size_t n_win = 8;
	const size_t max_lag = 12;
	const size_t winsize = 8;
	const size_t hopsize = 4;
	float bandwidths[8] =
		{1., 0.875, 0.75, 0.625, 0.5, 0.375, 0.25, 0.125};

	// required signal length is (8-1) * 4 + 8 + 12 = 48
	ck_assert_uint_eq(48u, ExpectedPaddedAudioLength(n_win, winsize,
							 max_lag, hopsize));

	// the result should be constant, regardless of the input value
	float expected_results[8] = {0.};
	for (size_t i = 0; i < n_win; i++){
		const float in = 0;
		float kernel_val = 0;
		EvaluateKernel(&in, &kernel_val, 1, bandwidths[i]);
		expected_results[i] = kernel_val*max_lag;
		// this should just be max_lag/(bandwidths[i]*sqrt(2*pi))
		// (if the kernel is a Gaussian). It might be wise to check
		// basic kernel properties in a different test
	}

	float const_signal_vals[5] = {-1., -0.25941, 0., 0.48706, 1.};
	for (int n = 0; n <5; n++){
		alignas(16) float x[48];
		for (size_t i = 0; i < 48; i++){
			x[i] = const_signal_vals[n];
		}
		// CRITICAL that results is initialized to zeros here.
		float results[8] = {0.};
		CalcSummedLagCorrentrograms(x, bandwidths, winsize, max_lag,
					    hopsize, n_win, results);
		compareArrayEntries_float(expected_results, results,
					  (int)n_win, 5e-7, 1, 0.);
	}
}
END_TEST


START_TEST(calcSummedLagCorrentrograms_sine_wave)
{
	// We can be more clever about computing the expected results for
	// periodic signals (we don't need to invoke NaiveSummedCorrentrograms
	// every single time we want to compute the correntrograms).

	const size_t n_win = 8;
	const size_t max_lag = 12;
	const size_t winsize = 8;
	const size_t hopsize = 4;
	float bandwidths[8] =
		{1., 0.875, 0.75, 0.625, 0.5, 0.375, 0.25, 0.125};

	// required signal length is (8-1) * 4 + 8 + 12 = 48
	ck_assert_uint_eq(48u, ExpectedPaddedAudioLength(n_win, winsize,
							 max_lag, hopsize));

	float input[48];
	double period = 1.0; // number of seconds
	int samplerate = 16; // samples per second
	genSineWave((void*) input, float_precision, 48, period, samplerate,
		    1.0, 0.);

	float expected_results[8] = {0.};
	NaiveSummedCorrentrograms(input, bandwidths, winsize, max_lag,
				  hopsize, n_win, expected_results);
	float results[8] = {0.}; // impotant to initialize to zeros
	CalcSummedLagCorrentrograms(input, bandwidths, winsize, max_lag,
				    hopsize, n_win, results);
	compareArrayEntries_float(expected_results, results,
				  (int)n_win, 5e-7, 1, 0.);
}

//========================================================

static float evaluate_gauss_(float x, float sigma){
	float norm = 1/(sigma*sqrtf(2*M_PI));
	return norm * expf(-0.5*x*x/(sigma*sigma));
}

START_TEST(kernel_accuracy)
{
	static const float rtol = 3.56e-2; // this is a property of the
	                                   // approximation
	float input_vals[6] = {0., 1., 2., 3., 4., 100.};
	bool truncated[6] = {false, false, false, false, false, true};

	// it's important that we use buffers, not just a single value. There
	// were previously bugs with this functionallity
	float input[8];
	float output[8];
	for (int n = 0; n < 6; n++){
		for (size_t index = 0; index < 8; index++){
			input[index] = input_vals[n];
		}

		EvaluateKernel(input, output, 8, 1.f);

		if (truncated[n]){
			for (size_t index = 0; index < 8; index++){
				ck_assert_float_eq(output[index], 0.f);
			}
		} else {
			float expected = evaluate_gauss_(input_vals[n], 1.f);
			for (size_t index = 0; index < 8; index++){
				float rel_err = ((output[index] - expected) /
						 expected);
				if (fabs(rel_err) >= rtol){
					ck_abort_msg(
						("Error at index %ld.\n"
						 "A Gaussian, with mu=0 & "
						 "sigma=1 at x = %f is %e.\n"
						 "The approximation gives %e. "
						 "The relative error, %e, "
						 "exeeds the tolerance, %e."),
						index, input_vals[n], expected,
						output[index], rel_err, rtol);
				}
			}
		}
	}
}

//========================================================
//error reporting tests

START_TEST(calcSummedLagCorrentrograms_overlapping_arrays)
{
	alignas(16) float input[210] = {0.}; // zero initialize the values
	size_t hopsize = 4;
	size_t max_lag = 12;
	size_t winsize = 16;
	size_t n_win = 3;

	{
		int rv = CalcSummedLagCorrentrograms(input, input,
						     winsize, max_lag,
						     hopsize, n_win,
						     input);
		ck_assert_int_ne(rv, ME_SUCCESS);
	}

	{
		int rv = CalcSummedLagCorrentrograms(input, input+200, winsize,
						     max_lag, hopsize, n_win,
						     input+5);
		ck_assert_int_ne(rv, ME_SUCCESS);
	}

	{
		int rv = CalcSummedLagCorrentrograms(input+64, input+63,
						     winsize, max_lag,
						     hopsize, n_win,
						     input+200);
		ck_assert_int_ne(rv, ME_SUCCESS);
	}

	{
		int rv = CalcSummedLagCorrentrograms(input, input+200,
						     winsize, max_lag,
						     hopsize, n_win,
						     input+201);
		ck_assert_int_ne(rv, ME_SUCCESS);
	}
}
END_TEST

START_TEST(calcSummedLagCorrentrograms_unaligned_input)
{
	alignas(16) float input[210] = {0.}; // zero initialize the values
	size_t hopsize = 4;
	size_t max_lag = 12;
	size_t winsize = 16;
	size_t n_win = 3;

	{
		int rv = CalcSummedLagCorrentrograms(input+1, input+200,
						     winsize, max_lag,
						     hopsize, n_win,
						     input+210);
		ck_assert_int_ne(rv, ME_SUCCESS);
	}
}
END_TEST

START_TEST(calcSummedLagCorrentrograms_non_multiple_of_4)
{
	alignas(16) float input[210] = {0.}; // zero initialize the values
	size_t hopsize = 4;
	size_t max_lag = 12;
	size_t winsize = 16;
	size_t n_win = 3;

	{
		int rv = CalcSummedLagCorrentrograms(input, input+200,
						     winsize+1, max_lag,
						     hopsize, n_win,
						     input+210);
		ck_assert_int_ne(rv, ME_SUCCESS);
	}

	{
		int rv = CalcSummedLagCorrentrograms(input, input+200,
						     winsize, max_lag+1,
						     hopsize, n_win,
						     input+210);
		ck_assert_int_ne(rv, ME_SUCCESS);
	}

	{
		int rv = CalcSummedLagCorrentrograms(input, input+200,
						     winsize, max_lag,
						     hopsize+1, n_win,
						     input+210);
		ck_assert_int_ne(rv, ME_SUCCESS);
	}
}
END_TEST


Suite * calcSummedLagCorrentrograms_suite(void)
{
	Suite *s = suite_create("calcSummedLagCorrentrograms");
	TCase *tc_kernel = tcase_create("kernel");
	tcase_add_test(tc_kernel, kernel_accuracy);

	TCase *tc_regression = tcase_create("Regression");
	tcase_add_test(tc_regression,
		       calcSummedLagCorrentrograms_uniform_signal);
	tcase_add_test(tc_regression,
		       calcSummedLagCorrentrograms_sine_wave);

	TCase *tc_error_handling = tcase_create("Error Handling");

	tcase_add_test(tc_error_handling,
		       calcSummedLagCorrentrograms_overlapping_arrays);
	tcase_add_test(tc_error_handling,
		       calcSummedLagCorrentrograms_unaligned_input);
	tcase_add_test(tc_error_handling,
		       calcSummedLagCorrentrograms_non_multiple_of_4);

	suite_add_tcase(s, tc_kernel);
	suite_add_tcase(s, tc_regression);
	suite_add_tcase(s, tc_error_handling);
	return s;
}
