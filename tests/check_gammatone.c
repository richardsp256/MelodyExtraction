#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../src/gammatoneFilter.h"


/* this is just a basic test to make sure Check is generated correctly. I plan 
 * to construct more of a framework structure presently reflected in 
 * gammatoneFilter.c
 */

START_TEST(test_sos_coefficients)
{
  double *coef = malloc(sizeof(double)*24);
  allPoleCoef(1000., 11025, coef);
  printf("%lf\n", coef[0]);
  double diff = abs(6.1031107e-02-coef[0]);
  diff/=6.1031107e-02;
  if (diff < 0){
    diff*=-1;
  }
  ck_assert_double_le(diff,1.e-5);
  free(coef);
}
END_TEST

Suite *gammatone_suite()
{
  Suite *s = suite_create("Gammatone");
  TCase *tc_coef = tcase_create("Coef");
  tcase_add_test(tc_coef,test_sos_coefficients);
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
