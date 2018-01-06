#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../src/gammatoneFilter.h"


/* this is just a basic test to make sure Check is generated correctly. I plan 
 * to construct more of a framework structure presently reflected in 
 * gammatoneFilter.c
 */

void compareArrayEntries_new(double *ref, double* other, int length,
			     double tol, int rel,double abs_zero_tol)
{
	// if relative < 1, then we compare absolute value of the absolute
	// difference between values
	// if relative > 1, we need to specially handle when the reference
	// value is zero. In that case, we look at the absolute differce and
	// compare it to abs_zero_tol
	
	int i;
	double diff;

	for (i = 0; i< length; i++){
		diff = abs(ref[i]-other[i]);
		if (rel >=1){
			if (ref[i] == 0){
				// we will just compute abs difference
				ck_assert_msg(diff <= abs_zero_tol,
					      ("ref[%d] = 0, comp[%d] = %e"
					       " has abs diff > %e\n"),
					      i, i, other[i],
					      abs_zero_tol);
				continue;
			}
			diff = diff/abs(ref[i]);
		}
		if (diff>tol){
			if (rel>=1){
				ck_abort_msg(("ref[%d] = %e, comp[%d] = %e"
					      " has rel dif > %e"),
					     i,ref[i],i,other[i], tol);
			} else {
				ck_abort_msg(("ref[%d] = %e, comp[%d] = %e"
					      " has abs dif > %e"),
					     i,ref[i],i,other[i], tol);
			}
		}		
	}
}


START_TEST(test_sos_coefficients)
{
	double *coef = malloc(sizeof(double)*24);
	allPoleCoef(1000., 11025, coef);
	double diff = abs(6.1031107e-02-coef[0]);
	diff/=6.1031107e-02;
	if (diff < 0){
		diff*=-1;
	}
	ck_assert_double_le(diff,1.e-5);
	free(coef);
}
END_TEST

START_TEST(test_sos_coefficients_new)
{
	double ref[] = {6.1031107e-02, -1.2118071e-01, 0., 1., -1.5590685e+00,
			8.5722460e-01, 5.5986645e-02, 2.3877628e-02, 0., 1.,
			-1.5590685e+00, 8.5722460e-01, 1.3816354e-01,
			-1.3629211e-01, 0., 1., -1.5590685e+00, 8.5722460e-01,
			1.2797372e-01, -7.3279486e-02, 0., 1., -1.5590685e+00,
			8.5722460e-01};
	double *coef = malloc(sizeof(double)*24);

	allPoleCoef(1000., 11025, coef);

	compareArrayEntries_new(ref, coef, 24, 1.e-5, 1, 1.e-5);
	free(coef);
}
END_TEST

Suite *gammatone_suite()
{
	Suite *s = suite_create("Gammatone");
	TCase *tc_coef = tcase_create("Coef");
	tcase_add_test(tc_coef,test_sos_coefficients);
	tcase_add_test(tc_coef,test_sos_coefficients_new);
	suite_add_tcase(s, tc_coef);
	return s;
}

int main(void){
	Suite *s = gammatone_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);;
	srunner_free(sr);
	if (number_failed == 0){
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
