#include <stdlib.h> // size_t
#include <stdio.h>
#include <stdalign.h>
#include <check.h>

#include "../src/transient/calcSummedLagCorrentrograms.h"
#include "../src/errors.h"

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

// For now, we are just adding tests of error reporting
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
	// create general test case (should be split into smaller tests)
	TCase *tc_error_handling = tcase_create("Error Handling");

	tcase_add_test(tc_error_handling,
		       calcSummedLagCorrentrograms_overlapping_arrays);
	tcase_add_test(tc_error_handling,
		       calcSummedLagCorrentrograms_unaligned_input);
	tcase_add_test(tc_error_handling,
		       calcSummedLagCorrentrograms_non_multiple_of_4);

	suite_add_tcase(s, tc_error_handling);
	return s;
}


int main(void){
	SRunner *sr = srunner_create(calcSummedLagCorrentrograms_suite());

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	if (number_failed == 0){
		return 0;
	} else {
		return 1;
	}
}
