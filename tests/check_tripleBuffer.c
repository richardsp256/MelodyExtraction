#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <limits.h>
#include "../src/tripleBuffer.h"

tripleBuffer *four_channel_tB;

void setup_emptyFourChanneltB(void){
	four_channel_tB = tripleBufferCreate(4, 56);
}

void teardown_FourChanneltB(void){
	tripleBufferDestroy(four_channel_tB);
}

START_TEST(test_tripleBufferCreate)
{
	ck_assert_int_eq(tripleBufferNumChannels(four_channel_tB), 4);
	ck_assert_int_eq(tripleBufferBufferLength(four_channel_tB), 56);
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),0);
	ck_assert_int_eq(tripleBufferIsTerminatedStream(four_channel_tB),0);
	ck_assert_int_eq(tripleBufferGetTerminalIndex(four_channel_tB),-1);
}
END_TEST

START_TEST(test_tripleBufferAddBuffers)
{
	for (int i= 0; i<3;i++){
		int temp = tripleBufferAddLeadingBuffer(four_channel_tB);
		ck_assert_int_eq(temp,1);
		ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),i+1);
	}
}
END_TEST

START_TEST(test_tripleBufferGetBufferPointer)
{
	tripleBufferAddLeadingBuffer(four_channel_tB);
	for (int i =0;i<4;i++) {
		float* temp =tripleBufferGetBufferPtr(four_channel_tB,0,i);
		ck_assert_ptr_nonnull(temp);
	}
}
END_TEST

void setUniqueBufferVals(tripleBuffer *tB, int numChannels, int bufferNum,
			 int bufferIndex)
{
	float* temp;
	int length = tripleBufferBufferLength(tB);
	ck_assert_int_ge(length,2);

	for (int i = 0; i < numChannels;i++){
		temp = tripleBufferGetBufferPtr(tB,bufferIndex, i);
		ck_assert_ptr_nonnull(temp);
		temp[0] = bufferNum;
		temp[1] = i;
		for (int j=2; j<numChannels;j++){
			temp[j] = 0.0;
		}
	}
}

void checkUniqueBufferVals(tripleBuffer *tB, int numChannels, int bufferNum,
			   int bufferIndex)
{
	float* temp;
	int length = tripleBufferBufferLength(tB);
	ck_assert_int_ge(length,2);

	for (int i = 0; i < numChannels;i++){
		temp = tripleBufferGetBufferPtr(tB,bufferIndex, i);
		ck_assert_ptr_nonnull(temp);
		ck_assert_float_eq(temp[0],bufferNum);
		ck_assert_float_eq(temp[1],i);
		for (int j=2; j<numChannels;j++){
			ck_assert_float_eq(temp[j], 0.0);
		}
	}
}

START_TEST(test_tripleBufferAdjustBuffers)
{
	for (int i= 0; i<3;i++){
		tripleBufferAddLeadingBuffer(four_channel_tB);
		setUniqueBufferVals(four_channel_tB, 4, i, i);
	}

	for (int j=0; j<3;j++){
		checkUniqueBufferVals(four_channel_tB, 4, j,j);
	}
		
}
END_TEST

START_TEST(test_tripleBufferCycle_Basic){

	for (int i= 0; i<3;i++){
		tripleBufferAddLeadingBuffer(four_channel_tB);
		setUniqueBufferVals(four_channel_tB, 4, i, i);
	}
	int cycle_result = tripleBufferCycle(four_channel_tB);
	ck_assert_int_eq(cycle_result, 1);
	checkUniqueBufferVals(four_channel_tB,4, 2, 1);
	checkUniqueBufferVals(four_channel_tB,4, 1, 0);
	checkUniqueBufferVals(four_channel_tB,4, 0, 2);
}
END_TEST

START_TEST(test_tripleBufferCycle_Complex){
	for (int i= 0; i<3;i++){
		tripleBufferAddLeadingBuffer(four_channel_tB);
		setUniqueBufferVals(four_channel_tB, 4, i, i);
	}

	for (int i= 3; i<8;i++){
		int cycle_result = tripleBufferCycle(four_channel_tB);
		ck_assert_int_eq(cycle_result, 1);
		tripleBufferAddLeadingBuffer(four_channel_tB);
		setUniqueBufferVals(four_channel_tB, 4, i, 2);
	}

	for (int i = 0; i<3; i++){
		checkUniqueBufferVals(four_channel_tB, 4, i+5, i);
	}
}
END_TEST


START_TEST(test_tripleBufferRemoveTrailingBuffer_Basic){
	for (int i= 0; i<3;i++){
		tripleBufferAddLeadingBuffer(four_channel_tB);
		setUniqueBufferVals(four_channel_tB, 4, i, i);
	}

	int temp;
	//remove buffer 1 of 3
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),2);
	checkUniqueBufferVals(four_channel_tB, 4, 1,0);
	checkUniqueBufferVals(four_channel_tB, 4, 2,1);

	// remove buffer 2 of 3
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),1);
	checkUniqueBufferVals(four_channel_tB, 4, 2,0);

	// remove buffer 3 of 3
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),0);
}
END_TEST

START_TEST(test_tripleBufferRemoveTrailingBuffer_AddRemove){
	// we may want to break this up into more tests.
	// the main purpose of this test is to ensure that addition and removals
	// of buffers do not interfere with each other
	for (int i= 0; i<3;i++){
		tripleBufferAddLeadingBuffer(four_channel_tB);
		setUniqueBufferVals(four_channel_tB, 4, i, i);
	}

	int temp;
	//remove buffer 3
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);
	

	// add buffer 3
	temp = tripleBufferAddLeadingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);
	setUniqueBufferVals(four_channel_tB, 4, 3, 2);
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),3);
	checkUniqueBufferVals(four_channel_tB, 4, 1,0);
	checkUniqueBufferVals(four_channel_tB, 4, 2,1);
	checkUniqueBufferVals(four_channel_tB, 4, 3,2);

	// remove buffers 3 and 2
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);

	// let's check that buffer 1 is the only remaining buffer
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),1);
	checkUniqueBufferVals(four_channel_tB, 4, 3,0);

	// add buffer 2
	temp = tripleBufferAddLeadingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);
	setUniqueBufferVals(four_channel_tB, 4, 4, 1);
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),2);
	checkUniqueBufferVals(four_channel_tB, 4, 3,0);
	checkUniqueBufferVals(four_channel_tB, 4, 4,1);

	// remove buffer 2
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);

	// let's check that buffer 1 is the only remaining buffer
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),1);
	checkUniqueBufferVals(four_channel_tB, 4, 4,0);

	// remove the final buffer
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);
	ck_assert_int_eq(tripleBufferNumBuffers(four_channel_tB),0);	
}
END_TEST

START_TEST(test_tripleBufferSetTerminalIndex)
{
	int temp;
	for (int i= 0; i<3;i++){
		tripleBufferAddLeadingBuffer(four_channel_tB);
		setUniqueBufferVals(four_channel_tB, 4, i, i);
	}
	tripleBufferSetTerminalIndex(four_channel_tB, 7);
	temp = tripleBufferRemoveTrailingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,1);

	temp = tripleBufferAddLeadingBuffer(four_channel_tB);
	ck_assert_int_eq(temp,0);

	temp = tripleBufferCycle(four_channel_tB);
	ck_assert_int_eq(temp, 0);
	
}
END_TEST

START_TEST(test_tripleBufferCreate_TooLarge)
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

START_TEST(test_tripleBufferCreate_ZeroChannels)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(0, 77175);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer with 0 channels.");
}
END_TEST

START_TEST(test_tripleBufferCreate_NegChannels)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(-5, 77175);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer with a negative number of channels.");
}
END_TEST

START_TEST(test_tripleBufferCreate_ZeroBufferLength)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(3, 0);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer with 0 buffer length.");
}
END_TEST

START_TEST(test_tripleBufferCreate_NegBufferLength)
{
	tripleBuffer *tB;

	tB = tripleBufferCreate(3, -5000);
	ck_assert_msg(tB == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a tripleBuffer with a negative buffer length.");
}
END_TEST

START_TEST(test_tripleBufferAddBuffers_TooMany)
{
	tripleBuffer *tB;
	tB = tripleBufferCreate(4, 56);
	for (int i=0;i<5;i++){
		int temp = tripleBufferAddLeadingBuffer(tB);
		if (i<3){
			ck_assert_int_eq(temp,1);
		} else {
			ck_assert_int_eq(temp,0);
		}
	}
	ck_assert_msg(tripleBufferNumBuffers(tB) == 3,
		      "Any additional buffer after tripleBuffer already "
		      "contains three buffers should be ignored.");
	tripleBufferDestroy(tB);
}
END_TEST

START_TEST(test_tripleBufferCycle_Empty)
{
	tripleBuffer *tB;
	tB = tripleBufferCreate(4, 56);
	ck_assert_int_eq(tripleBufferCycle(tB),0);
}
END_TEST

START_TEST(test_tripleBufferRemoveTrailingBuffer_Empty)
{
	tripleBuffer *tB;
	tB = tripleBufferCreate(4, 56);
	ck_assert_int_eq(tripleBufferRemoveTrailingBuffer(tB),0);
}
END_TEST

START_TEST(test_tripleBufferSetTerminalIndex_Empty)
{
	tripleBuffer *tB;
	tB = tripleBufferCreate(4, 56);
	ck_assert_int_eq(tripleBufferSetTerminalIndex(tB,55),0);
}
END_TEST

START_TEST(test_tripleBufferSetTerminalIndex_NegIndex)
{
	tripleBuffer *tB;
	tB = tripleBufferCreate(4, 56);
	tripleBufferAddLeadingBuffer(tB);
	ck_assert_int_eq(tripleBufferSetTerminalIndex(tB,-1),0);
}
END_TEST

START_TEST(test_tripleBufferSetTerminalIndex_TooLarge)
{
	tripleBuffer *tB;
	tB = tripleBufferCreate(4, 56);
	tripleBufferAddLeadingBuffer(tB);
	ck_assert_int_eq(tripleBufferSetTerminalIndex(tB,56),0);
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
	tcase_add_checked_fixture(tc_core, setup_emptyFourChanneltB,
				  teardown_FourChanneltB);
	tcase_add_test(tc_core,test_tripleBufferCreate);
	tcase_add_test(tc_core,test_tripleBufferAddBuffers);
	tcase_add_test(tc_core,test_tripleBufferGetBufferPointer);
	tcase_add_test(tc_core,test_tripleBufferAdjustBuffers);
	tcase_add_test(tc_core,test_tripleBufferCycle_Basic);
	tcase_add_test(tc_core,test_tripleBufferCycle_Complex);
	tcase_add_test(tc_core,test_tripleBufferRemoveTrailingBuffer_Basic);
	tcase_add_test(tc_core,test_tripleBufferRemoveTrailingBuffer_AddRemove);
	tcase_add_test(tc_core,test_tripleBufferSetTerminalIndex);
	suite_add_tcase(s, tc_core);

	/* Limits test case */
	tc_limits = tcase_create("Limits");
	tcase_add_test(tc_limits,test_tripleBufferCreate_TooLarge);
	tcase_add_test(tc_limits,test_tripleBufferCreate_ZeroChannels);
	tcase_add_test(tc_limits,test_tripleBufferCreate_NegChannels);
	tcase_add_test(tc_limits,test_tripleBufferCreate_ZeroBufferLength);
	tcase_add_test(tc_limits,test_tripleBufferCreate_NegBufferLength);
	tcase_add_test(tc_limits,test_tripleBufferAddBuffers_TooMany);
	tcase_add_test(tc_limits,test_tripleBufferCycle_Empty);
	tcase_add_test(tc_limits,test_tripleBufferRemoveTrailingBuffer_Empty);

	tcase_add_test(tc_limits,test_tripleBufferSetTerminalIndex_Empty);
	tcase_add_test(tc_limits,test_tripleBufferSetTerminalIndex_NegIndex);
	tcase_add_test(tc_limits,test_tripleBufferSetTerminalIndex_TooLarge);

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
