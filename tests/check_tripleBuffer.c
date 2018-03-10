#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <limits.h>
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

START_TEST(test_tripleBuffer_TooLarge)
{
	tripleBuffer *tB;
	int bufferLength;

	bufferLength = INT_MAX/64 +1;
	tB = tripleBufferCreate(64, bufferLength);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer which would require indices larger than "
		      "the maximum value of int.");
}
END_TEST

START_TEST(test_tripleBuffer_ZeroChannels)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(0, 77175);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer with 0 channels.");
}
END_TEST

START_TEST(test_tripleBuffer_NegChannels)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(-5, 77175);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer with a negative number of channels.");
}
END_TEST

START_TEST(test_tripleBuffer_ZeroBufferLength)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(3, 0);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer with 0 buffer length.");
}
END_TEST

START_TEST(test_tripleBuffer_NegBufferLength)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(3, -5000);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer with a negative buffer length.");
}
END_TEST

Suite *tripleBuffer_suite(void)
{
	Suite *s;
	TCase *tc_core;
	TCase *tc_limits;

	s = suite_create("tripleBuffer");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_add_test(tc_core,test_tripleBufferCreate);
	suite_add_tcase(s, tc_core);

	/* Limits test case */
	tc_limits = tcase_create("Limits");
	tcase_add_test(tc_limits,test_tripleBuffer_TooLarge);
	tcase_add_test(tc_limits,test_tripleBuffer_ZeroChannels);
	tcase_add_test(tc_limits,test_tripleBuffer_NegChannels);
	tcase_add_test(tc_limits,test_tripleBuffer_ZeroBufferLength);
	tcase_add_test(tc_limits,test_tripleBuffer_NegBufferLength);
	suite_add_tcase(s,tc_limits);
	
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
