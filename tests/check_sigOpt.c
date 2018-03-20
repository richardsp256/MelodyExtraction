#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <limits.h>
#include "../src/sigOpt.h"

bufferIndex *buff0Ind4;

void setup_buff0Ind4(void){
	buff0Ind4 = bufferIndexCreate(0,4,6);
}

void teardown_buff0Ind4(void){
	bufferIndexDestroy(buff0Ind4);
}

START_TEST(test_bufferIndexCreate)
{
	ck_assert_int_eq(bufferIndexGetBufferNum(buff0Ind4),0);
	ck_assert_int_eq(bufferIndexGetIndex(buff0Ind4),4);
	ck_assert_int_eq(bufferIndexGetBufferLength(buff0Ind4),6);	
}
END_TEST


START_TEST(test_bufferIndexIncrement)
{
	int temp;
	temp = bufferIndexIncrement(buff0Ind4);
	ck_assert_int_eq(temp,1);
	ck_assert_int_eq(bufferIndexGetBufferNum(buff0Ind4),0);
	ck_assert_int_eq(bufferIndexGetIndex(buff0Ind4),5);
	temp = bufferIndexIncrement(buff0Ind4);
	ck_assert_int_eq(temp,1);
	ck_assert_int_eq(bufferIndexGetBufferNum(buff0Ind4),1);
	ck_assert_int_eq(bufferIndexGetIndex(buff0Ind4),0);
}
END_TEST

START_TEST(test_bufferIndexAdvanceBuffer)
{
	int temp;
	temp = bufferIndexAdvanceBuffer(buff0Ind4);
	ck_assert_int_eq(temp,1);
	ck_assert_int_eq(bufferIndexGetBufferNum(buff0Ind4),-1);
	
}
END_TEST

START_TEST(test_bufferIndexAddScalarIndex)
{
	bufferIndex *bI;
	// same bufferNum
	bI = bufferIndexAddScalarIndex(buff0Ind4, 1);
	ck_assert_ptr_nonnull(bI);
	ck_assert_int_eq(bufferIndexGetBufferNum(bI),0);
	ck_assert_int_eq(bufferIndexGetIndex(bI),5);
	bufferIndexDestroy(bI);

	// bufferNum 1 Index 0
	bI = bufferIndexAddScalarIndex(buff0Ind4, 2);
	ck_assert_ptr_nonnull(bI);
	ck_assert_int_eq(bufferIndexGetBufferNum(bI),1);
	ck_assert_int_eq(bufferIndexGetIndex(bI),0);
	bufferIndexDestroy(bI);

	// bufferNum2 Index 3
	bI = bufferIndexAddScalarIndex(buff0Ind4, 11);
	ck_assert_ptr_nonnull(bI);
	ck_assert_int_eq(bufferIndexGetBufferNum(bI),2);
	ck_assert_int_eq(bufferIndexGetIndex(bI),3);
	bufferIndexDestroy(bI);

	// failure - neg num
	bI = bufferIndexAddScalarIndex(buff0Ind4, -1);
	ck_assert_ptr_null(bI);
}
END_TEST

// test Case for comparison
// test Case for adding indices


bufferIndex *buff0Ind0;
bufferIndex *buff0Ind5;
bufferIndex *buffNeg1Ind2;
bufferIndex *buff1Ind3;
bufferIndex *buff0Ind4DiffLength;

void setup_comparisons(void){
	setup_buff0Ind4();
	buff0Ind0 = bufferIndexCreate(0,0,6);
	buff0Ind5 = bufferIndexCreate(0,5,6);
	buffNeg1Ind2 = bufferIndexCreate(-1,2,6);
	buff1Ind3 = bufferIndexCreate(0,5,6);
	buff0Ind4DiffLength = bufferIndexCreate(0,4,7);
}

void teardown_comparisons(void){
	teardown_buff0Ind4();
	bufferIndexDestroy(buff0Ind0);
	bufferIndexDestroy(buff0Ind5);
	bufferIndexDestroy(buffNeg1Ind2);
	bufferIndexDestroy(buff1Ind3);
	bufferIndexDestroy(buff0Ind4DiffLength);
}

START_TEST(test_bufferIndexEq)
{
	bufferIndex* bI= bufferIndexCreate(0,4,6);
	ck_assert_int_eq(bufferIndexEq(buff0Ind4, bI), 1);
	ck_assert_int_eq(bufferIndexEq(buff0Ind0, bI), 0);
	ck_assert_int_eq(bufferIndexEq(buff0Ind5, bI), 0);
	ck_assert_int_eq(bufferIndexEq(buffNeg1Ind2, bI), 0);
	ck_assert_int_eq(bufferIndexEq(buff1Ind3, bI), 0);
	ck_assert_int_eq(bufferIndexEq(buff0Ind4DiffLength, bI), 0);
	bufferIndexDestroy(bI);
}
END_TEST

START_TEST(test_bufferIndexNe)
{
	bufferIndex* bI= bufferIndexCreate(0,4,6);
	ck_assert_int_eq(bufferIndexNe(buff0Ind4, bI), 0);
	ck_assert_int_eq(bufferIndexNe(buff0Ind0, bI), 1);
	ck_assert_int_eq(bufferIndexNe(buff0Ind5, bI), 1);
	ck_assert_int_eq(bufferIndexNe(buffNeg1Ind2, bI), 1);
	ck_assert_int_eq(bufferIndexNe(buff1Ind3, bI), 1);
	ck_assert_int_eq(bufferIndexNe(buff0Ind4DiffLength, bI), 1);
	bufferIndexDestroy(bI);
}
END_TEST

START_TEST(test_bufferIndexGt)
{
	bufferIndex* bI= bufferIndexCreate(0,4,6);
	ck_assert_int_eq(bufferIndexGt(buff0Ind4, bI), 0);
	ck_assert_int_eq(bufferIndexGt(buff0Ind0, bI), 0);
	ck_assert_int_eq(bufferIndexGt(buff0Ind5, bI), 1);
	ck_assert_int_eq(bufferIndexGt(buffNeg1Ind2, bI), 0);
	ck_assert_int_eq(bufferIndexGt(buff1Ind3, bI), 1);
	ck_assert_int_eq(bufferIndexGt(buff0Ind4DiffLength, bI), 0);
	bufferIndexDestroy(bI);
}
END_TEST

START_TEST(test_bufferIndexGe)
{
	bufferIndex* bI= bufferIndexCreate(0,4,6);
	ck_assert_int_eq(bufferIndexGe(buff0Ind4, bI), 1);
	ck_assert_int_eq(bufferIndexGe(buff0Ind0, bI), 0);
	ck_assert_int_eq(bufferIndexGe(buff0Ind5, bI), 1);
	ck_assert_int_eq(bufferIndexGe(buffNeg1Ind2, bI), 0);
	ck_assert_int_eq(bufferIndexGe(buff1Ind3, bI), 1);
	ck_assert_int_eq(bufferIndexGe(buff0Ind4DiffLength, bI), 0);
	bufferIndexDestroy(bI);
}
END_TEST

START_TEST(test_bufferIndexLt)
{
	bufferIndex* bI= bufferIndexCreate(0,4,6);
	ck_assert_int_eq(bufferIndexLt(buff0Ind4, bI), 0);
	ck_assert_int_eq(bufferIndexLt(buff0Ind0, bI), 1);
	ck_assert_int_eq(bufferIndexLt(buff0Ind5, bI), 0);
	ck_assert_int_eq(bufferIndexLt(buffNeg1Ind2, bI), 1);
	ck_assert_int_eq(bufferIndexLt(buff1Ind3, bI), 0);
	ck_assert_int_eq(bufferIndexLt(buff0Ind4DiffLength, bI), 0);
	bufferIndexDestroy(bI);
}
END_TEST

START_TEST(test_bufferIndexLe)
{
	bufferIndex* bI= bufferIndexCreate(0,4,6);
	ck_assert_int_eq(bufferIndexLe(buff0Ind4, bI), 1);
	ck_assert_int_eq(bufferIndexLe(buff0Ind0, bI), 1);
	ck_assert_int_eq(bufferIndexLe(buff0Ind5, bI), 0);
	ck_assert_int_eq(bufferIndexLe(buffNeg1Ind2, bI), 1);
	ck_assert_int_eq(bufferIndexLe(buff1Ind3, bI), 0);
	ck_assert_int_eq(bufferIndexLe(buff0Ind4DiffLength, bI), 0);
	bufferIndexDestroy(bI);
}
END_TEST

START_TEST(test_bufferIndexCreate_NegIndex)
{
	bufferIndex *bI = bufferIndexCreate(0,-1,6);
	ck_assert_msg(bI == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a bufferIndex with a negative Index.");
}
END_TEST

START_TEST(test_bufferIndexCreate_NegBufferSize)
{
	bufferIndex *bI = bufferIndexCreate(0, 5, -1);
	ck_assert_msg(bI == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a buffer length with a negative value.");
}
END_TEST

START_TEST(test_bufferIndexCreate_ZeroBufferSize)
{
	bufferIndex *bI = bufferIndexCreate(0, 0, 0);
	ck_assert_msg(bI == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a buffer length with zero length.");
}
END_TEST

START_TEST(test_bufferIndexCreate_IndexBufferSize)
{
	bufferIndex *bI = bufferIndexCreate(0,6,6);
	ck_assert_msg(bI == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a bufferIndex with an index equal to the buffer size.");
	bI = bufferIndexCreate(0,7,6);
	ck_assert_msg(bI == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a bufferIndex with an index greater than the buffer "
		      "size.");
}
END_TEST



START_TEST(test_sigOptCreate)
{
	sigOpt *sO =sigOptCreate(1,33075/2,55,22,33075/2,4,1.06);
	ck_assert_ptr_nonnull(sO);
	sigOptDestroy(sO);
}
END_TEST

START_TEST(test_sigOptCreate_badVariableValue)
{
	sigOpt *sO =sigOptCreate(-1,33075/2,55,22,33075/2,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_negWinSize)
{
	sigOpt *sO =sigOptCreate(1,-33075/2,55,22,33075/2,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroWinSize)
{
	sigOpt *sO =sigOptCreate(1,0,55,22,33075/2,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_negHopsize)
{
	sigOpt *sO =sigOptCreate(1,33075/2,-55,22,33075/2,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroHopsize)
{
	sigOpt *sO =sigOptCreate(1,33075/2,0,22,33075/2,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_negStartIndex)
{
	sigOpt *sO =sigOptCreate(1,33075/2,55,-22,33075/2,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroStartIndex)
{
	sigOpt *sO =sigOptCreate(1,33075/2,55,0,33075/2,4,1.06);
	ck_assert_ptr_nonnull(sO);
	sigOptDestroy(sO);
}
END_TEST

START_TEST(test_sigOptCreate_StartIndexWinSize)
{
	sigOpt *sO =sigOptCreate(1,33075/2,55,33075/2,33075/2,4,1.06);
	ck_assert_msg(sO == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a sigOpt with a startIndex>= winSize.");
}
END_TEST

START_TEST(test_sigOptCreate_negBufferLength)
{
	sigOpt *sO =sigOptCreate(1,33075/2,55,22,-33075/2,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroBufferLength)
{
	sigOpt *sO =sigOptCreate(1,33075/2,55,22,0,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_negChannel)
{
	sigOpt *sO =sigOptCreate(1,33075/2,55,22,33075/2,-4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroChannel)
{
	sigOpt *sO =sigOptCreate(1,33075/2,55,22,33075/2,0,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

Suite *sigOpt_suite(void)
{
	Suite *s;
	TCase *tc_sOcore;
	TCase *tc_sOlimits;
	TCase *tc_bIcore;
	TCase *tc_bIlimits;
	TCase *tc_bIcomp;

	s = suite_create("sigOpt");

	/* Core test case */
	tc_sOcore = tcase_create("sigOpt Core");
	tcase_add_test(tc_sOcore,test_sigOptCreate);
	suite_add_tcase(s, tc_sOcore);

	tc_sOlimits = tcase_create("sigOpt Limits");
	tcase_add_test(tc_sOlimits,test_sigOptCreate_badVariableValue);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negWinSize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroWinSize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negHopsize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroHopsize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negStartIndex);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroStartIndex);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_StartIndexWinSize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negBufferLength);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroBufferLength);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negChannel);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroChannel);
	suite_add_tcase(s, tc_sOlimits);

	tc_bIcore = tcase_create("bufferIndex Core");
	tcase_add_checked_fixture(tc_bIcore, setup_buff0Ind4,
				  teardown_buff0Ind4);
	tcase_add_test(tc_bIcore,test_bufferIndexCreate);
	tcase_add_test(tc_bIcore,test_bufferIndexIncrement);
	tcase_add_test(tc_bIcore,test_bufferIndexAdvanceBuffer);
	tcase_add_test(tc_bIcore,test_bufferIndexAddScalarIndex);
	suite_add_tcase(s, tc_bIcore);

	tc_bIcomp = tcase_create("bufferIndex Comparisons");
	tcase_add_checked_fixture(tc_bIcomp, setup_comparisons,
				  teardown_comparisons);
	tcase_add_test(tc_bIcomp,test_bufferIndexEq);
	tcase_add_test(tc_bIcomp,test_bufferIndexNe);
	tcase_add_test(tc_bIcomp,test_bufferIndexGt);
	tcase_add_test(tc_bIcomp,test_bufferIndexGe);
	tcase_add_test(tc_bIcomp,test_bufferIndexLt);
	tcase_add_test(tc_bIcomp,test_bufferIndexLe);
	suite_add_tcase(s,tc_bIcomp);

	tc_bIlimits = tcase_create("bufferIndex Limits");
	tcase_add_test(tc_bIlimits,test_bufferIndexCreate_NegIndex);
	tcase_add_test(tc_bIlimits,test_bufferIndexCreate_NegBufferSize);
	tcase_add_test(tc_bIlimits,test_bufferIndexCreate_ZeroBufferSize);
	tcase_add_test(tc_bIlimits,test_bufferIndexCreate_IndexBufferSize);
	suite_add_tcase(s, tc_bIlimits);

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
