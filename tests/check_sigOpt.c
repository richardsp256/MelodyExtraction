#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <limits.h>
#include "../src/sigOpt.h"


START_TEST(test_sigOptChannelCreate)
{
	sigOptChannel *sOC =sigOptChannelCreate(1, 33075, 55,
						22, 22,
						33075/2, 1.06);
	sigOptChannelDestroy(sOC);
}
END_TEST

Suite *sigOpt_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("sigOptChannel");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_add_test(tc_core,test_sigOptChannelCreate);
	suite_add_tcase(s, tc_core);
	
	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;
	s = sigOpt_suite();
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
