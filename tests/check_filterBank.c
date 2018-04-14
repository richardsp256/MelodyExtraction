#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <limits.h>
#include "../src/tripleBuffer.h"
#include "../src/filterBank.h"


/* This set of tests includes only a very simplistic test of basic 
 * functionallity.
 *
 * This set of tests is not designed to be as exhaustive as those for sigOpt or 
 * tripleBuffer. (This is mainly because the functionallity is simpler, and we 
 * should be probably be mocking interactions with tripleBuffer)
 */

START_TEST(test_filterBankCreate)
{
	filterBank *fB = filterBankNew(4, 47, 9, 11025, 80, 4000);
	ck_assert_ptr_nonnull(fB);
	ck_assert_int_eq(filterBankFirstChunkLength(fB), 47);
	ck_assert_int_eq(filterBankNormalChunkLength(fB), 38);
	filterBankDestroy(fB);
}
END_TEST

Suite *filterBank_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("filterBank");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_add_test(tc_core,test_filterBankCreate);

	suite_add_tcase(s,tc_core);
	
	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;
	s = filterBank_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	if (number_failed == 0){
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
};
