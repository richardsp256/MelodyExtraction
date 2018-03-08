#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../src/tripleBuffer.h"

START_TEST(test_tripleBufferCreate)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(4, 56);
	ck_assert_int_eq(tripleBufferNumChannels(tB), 4);
	ck_assert_int_eq(tripleBufferBufferLength(tB), 56);
	tripleBufferDestroy(tB);

}
END_TEST

Suite *tripleBuffer_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("tripleBuffer");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_add_test(tc_core,test_tripleBufferCreate);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;
	s = tripleBuffer_suite();
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
