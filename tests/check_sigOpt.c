#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <limits.h>
#include "../src/sigOpt.h"
#include "../src/simpleDetFunc.h"
#include "doubleArrayTesting.h"

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
	// using a buffer size of ~70 ms for samplerate of 11025 Hz
	// since 55 samples is ~ 5ms. This means the window size is 770
	// samples
	sigOpt *sO =sigOptCreate(770,55,22,4,1.06);
	ck_assert_ptr_nonnull(sO);
	ck_assert_int_eq(sigOptGetBufferLength(sO),440);
	ck_assert_int_eq(sigOptGetSigmasPerBuffer(sO),8);
	sigOptDestroy(sO);
}
END_TEST


START_TEST(test_sigOptSetTerminationIndex)
{
	int temp;
	sigOpt *sO =sigOptCreate(770,55,22,4,1.06);
	ck_assert_ptr_nonnull(sO);
	temp = sigOptSetTerminationIndex(sO,-1);
	ck_assert_msg(temp != 1,
		      "The termination index should not be set to a negative "
		      "index\n");
	temp = sigOptSetTerminationIndex(sO,440);
	ck_assert_msg(temp != 1,
		      "The termination index should not be set to an index "
		      "equal to the bufferLength\n");
	temp = sigOptSetTerminationIndex(sO,463);
	ck_assert_msg(temp != 1,
		      "The termination index should not be set to an index "
		      "greater than the bufferLength\n");
	temp = sigOptSetTerminationIndex(sO,4);
	ck_assert_msg(temp != 1,
		      "The termination index should not be set to an index "
		      "less than the hopsize if the entire stream is shorter "
		      "than the a single hopsize.\n");
	temp = sigOptSetTerminationIndex(sO,55);
	ck_assert_int_eq(temp,1);
	temp = sigOptSetTerminationIndex(sO,59);
	ck_assert_msg(temp != 1,
		      "The termination index should not be set to any other "
		      "values if it has already been set to an index\n");
	sigOptDestroy(sO);
}
END_TEST


START_TEST(test_sigOptCreate_negWinSize)
{
	sigOpt *sO =sigOptCreate(-770,55,22,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroWinSize)
{
	sigOpt *sO =sigOptCreate(0,55,22,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_negHopsize)
{
	sigOpt *sO =sigOptCreate(770,-55,22,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroHopsize)
{
	sigOpt *sO =sigOptCreate(770,0,22,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_negScaleFactor)
{
	sigOpt *sO =sigOptCreate(770,55,22,4,-1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroScaleFactor)
{
	sigOpt *sO =sigOptCreate(770,0,22,4,0);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_negInitialOffset)
{
	sigOpt *sO =sigOptCreate(770,55,-22,4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroInitialOffset)
{
	sigOpt *sO =sigOptCreate(770,55,0,4,1.06);
	ck_assert_ptr_nonnull(sO);
	sigOptDestroy(sO);
}
END_TEST

START_TEST(test_sigOptCreate_InitialOffsetWinSize)
{
	sigOpt *sO =sigOptCreate(770,55,770,4,1.06);
	ck_assert_msg(sO == NULL,
		      "NULL should be returned as result of attempt to create "
		      "a sigOpt with a startIndex>= winSize.");
}
END_TEST

START_TEST(test_sigOptCreate_negChannel)
{
	sigOpt *sO =sigOptCreate(770,55,22,-4,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST

START_TEST(test_sigOptCreate_zeroChannel)
{
	sigOpt *sO =sigOptCreate(770,55,22,0,1.06);
	ck_assert_ptr_null(sO);
}
END_TEST


/* The next set of tests will actually demonstrate sigOpt in action. */
double *dblrollSigma(int initialOffset, int hopsize, float scaleFactor,
		     int winSize, int dataLength, int numWindows,
		     float *input, int testFunc)
{
	ck_assert_ptr_nonnull(input);
	
	float *fltSigma= malloc(sizeof(float)*numWindows);
	if (testFunc == 1){
		int temp = sigOptFullRollSigma(initialOffset, hopsize,
					       scaleFactor, winSize, dataLength,
					       numWindows, input, &fltSigma);
		if (temp < 0){
			free(fltSigma);
			return NULL;
		}
	} else {
		fflush(stdout);
		rollSigma(initialOffset, hopsize, scaleFactor, winSize,
			  dataLength, numWindows, input, &fltSigma);
	}
	double *result = NULL;
	float_to_double_array(fltSigma, numWindows, &result);
	free(fltSigma);
	return result;
}

float* fltInput;
int inputLength;

void setup_sigOptRunExamples(void){
	double *original_input;
	inputLength = readDoubleArray(("tests/test_files/roll_sigma/"
				       "roll_sigma_sample_input"),
				      isLittleEndian(), &original_input);
	//inputLength is 51
	double_to_float_array(original_input, inputLength, &fltInput);
	free(original_input);
}

void teardown_sigOptRunExamples(void){
	free(fltInput);
}


START_TEST(test_sigOptFullRollSigma_Simple)
{
	// in this version, I want to deliberately break up the evaluation so
	// we can pinpoint what doesn't work if the test fails.
	
	int numWindows = 10; 
	int initialOffset = 1;
	int winSize = 15;
	int hopsize = 5;

	// we are not using all 51 entries
	inputLength = 49;

	double *reference = dblrollSigma(initialOffset, hopsize, 1.06,
					 winSize, inputLength, numWindows,
					 fltInput, 0); //rollSigma

	// this time we will run sigOpt manually for debugging.
	sigOpt *sO = sigOptCreate(winSize, hopsize, initialOffset, 1,
				  1.06);
	ck_assert_ptr_nonnull(sO);
	int bufferLength = sigOptGetBufferLength(sO);
	ck_assert_int_eq(bufferLength,15);
	int sigPerBuffer = sigOptGetSigmasPerBuffer(sO);
	ck_assert_int_eq(sigPerBuffer,3);

	float *centralBuffer = fltInput;
	int intermed_result = sigOptSetup(sO, 0, centralBuffer);
	ck_assert_int_eq(intermed_result,1);
	float *leadingBuffer = fltInput + bufferLength;
	float *trailingBuffer = NULL;

	float sig = sigOptAdvanceWindow(sO, NULL,
					centralBuffer,leadingBuffer,0);
	ck_assert_float_eq(sig,(float)(reference[0]));

	// now the window is centered on index 6
	// the left edge extends to 0 (it theoretically includes index -1)
	// the stop index is index 14 (of the stream and the central Buffer)
	sig = sigOptAdvanceWindow(sO, NULL, centralBuffer, leadingBuffer,
				  0);
	ck_assert_float_eq(sig,(float)(reference[1]));

	// now the window is centered at index 11 (the 11th index of the
	// central buffer)
	// at this point the left edge should now include index 4 and the right
	// edge should stop at index 19 (i.e. index 19 should not be included).
	// In other words the right edge should stop at index 4 of the second
	// (central) buffer
	// note that this is the first time, we will have a complete window.
	sig = sigOptAdvanceWindow(sO, NULL, centralBuffer, leadingBuffer,
				  0);
	ck_assert_float_eq(sig,(float)(reference[2]));

	// now for advancement across buffers
	trailingBuffer = centralBuffer;
	centralBuffer = leadingBuffer;
	leadingBuffer = fltInput + 2*bufferLength;
	sigOptAdvanceBuffer(sO);


	// advance current window. The final properties of the window after
	// advancement are:
	// Part of window  Index of stream    In buffer  Index of buffer
	// center          16                 central    1
	// left edge       9                  trailing   9
	// stop index      24                 central    9
	sig = sigOptAdvanceWindow(sO, trailingBuffer, centralBuffer,
				  leadingBuffer, 0);
	ck_assert_float_eq(sig,(float)(reference[3]));

	// advance current window
	// Part of window  Index of stream    In buffer  Index of buffer
	// center          21                 central    6
	// left edge       14                 trailing   14
	// stop index      29                 central    14
	sig = sigOptAdvanceWindow(sO, trailingBuffer, centralBuffer,
				  leadingBuffer, 0);
	ck_assert_float_eq(sig,(float)(reference[4]));

	// advance current window
	// Part of window  Index of stream    In buffer  Index of buffer
	// center          26                 central    11
	// left edge       19                 central    4
	// stop index      34                 leading    4
	sig = sigOptAdvanceWindow(sO, trailingBuffer, centralBuffer,
				  leadingBuffer, 0);
	ck_assert_float_eq(sig,(float)(reference[5]));


	// advance across buffers
	trailingBuffer = centralBuffer;
	centralBuffer = leadingBuffer;
	leadingBuffer = fltInput + 3*bufferLength;
	sigOptAdvanceBuffer(sO);
	
	// set termination index to 4 - this sets the length of the entire
	// stream to 49
	sigOptSetTerminationIndex(sO,4);

	// advance current window. The final properties of the window after
	// advancement are:
	// Part of window  Index of stream    In buffer  Index of buffer
	// center          31                 central    1
	// left edge       24                 trailing   9
	// stop index      39                 central    9
	sig = sigOptAdvanceWindow(sO, trailingBuffer, centralBuffer,
				  leadingBuffer, 0);
	ck_assert_float_eq(sig,(float)(reference[6]));
	
	// advance current window
	// Part of window  Index of stream    In buffer  Index of buffer
	// center          35                 central    6
	// left edge       29                 trailing   14
	// stop index      44                 central    14
	sig = sigOptAdvanceWindow(sO, trailingBuffer, centralBuffer,
				  leadingBuffer, 0);
	ck_assert_float_eq(sig,(float)(reference[7]));

	// advance current window
	// Part of window  Index of stream    In buffer  Index of buffer
	// center          41                 central    11
	// left edge       34                 central    4
	// stop index      49                 leading    4
	sig = sigOptAdvanceWindow(sO, trailingBuffer, centralBuffer,
				  leadingBuffer, 0);
	ck_assert_float_eq(sig,(float)(reference[8]));

	// we advance across buffers
	trailingBuffer = centralBuffer;
	centralBuffer = leadingBuffer;
	leadingBuffer = NULL;
	sigOptAdvanceBuffer(sO);

	// we advance the current window, calculating the final sigma for the
	// final buffer (now the central buffer). The final properties of the
	// window after advancement are given below. Values in the parenthesis
	// give the values if there stream had not terminated:
	// Part of window  Index of stream    In buffer  Index of buffer
	// center          46                 central    1
	// left edge       39                 trailing   9
	// stop index      49 (54)            central    4 (9)
	sig = sigOptAdvanceWindow(sO, trailingBuffer, centralBuffer,
				  leadingBuffer, 0);
	ck_assert_float_eq(sig,(float)(reference[9]));

	sigOptDestroy(sO);
	free(reference);
}
END_TEST

void sigOptFullRollSigma_testHelper(int initialOffset, int hopsize,
				    float scaleFactor, int winSize,
				    int inputLength, int numWindows,
				    float *fltInput){

	double *reference = dblrollSigma(initialOffset, hopsize, scaleFactor,
					 winSize, inputLength, numWindows,
					 fltInput, 0); //rollSigma
	double *result = dblrollSigma(initialOffset, hopsize, scaleFactor,
				      winSize, inputLength, numWindows,
				      fltInput, 1); //sigOptRollSigma
	
	
	ck_assert_ptr_nonnull(result);

	compareArrayEntries(reference, result, numWindows, 1.e-5, 1, 1.e-5);

	free(reference);
	free(result);
}

START_TEST(test_sigOptFullRollSigma_SimpleSmallerTermInd){
	// in the Simple version, the Termination Index was exactly equal to
	// the stop index of the second to last window.
	// Here we are testing what happens if Termination Index is never equal
	// to the last of a window.

	int numWindows = 10;
	int initialOffset = 1;
	int winSize = 15;
	int hopsize = 5;

	// we are not using all 51 entries -> reducing it to 48
	inputLength = 48;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_SimpleLargerOffset){
	// Here we are testing a slightly larger larger index of 3

	int numWindows = 10; // May be worth testing one more window
	int initialOffset = 6;
	int winSize = 15;
	int hopsize = 5;

	// we are not using all 51 entries -> reducing it to 49
	inputLength = 49;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);

} END_TEST

START_TEST(test_sigOptFullRollSigma_Even){
	// Here we are testing an even hopsize

	int numWindows = 12; // May be worth testing one more window
	int initialOffset = 6;
	int winSize = 16;
	int hopsize = 4;

	// we are using all 51 entries
	
	inputLength = 50;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_EvenSmallStream){
	// Here we are adding a unit test to try to demonstrate the edge case
	// that motivated us to use the left edge counter
	// this might not be quite right - but it did test the case where the
	// termination index was larger than the stop index in value - (i.e.
	// the stop index was the 1st index of the leading buffer and the
	// termination index was the 9th index of the central buffer)

	int numWindows = 3;
	int initialOffset = 2;
	int winSize = 16;
	int hopsize = 4;

	// we are using 9 entries
	inputLength = 9;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_EvenExactFill){
	// Here we are testing the case where the steam is perfectly evenly
	// divided between streams

	int numWindows = 12;
	int initialOffset = 6;
	int winSize = 16;
	int hopsize = 4;
	
	inputLength = 48;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_3Buffers){
	// Here we are testing sigOpt using 3 Buffers

	int numWindows = 10;
	int initialOffset = 6;
	int winSize = 16;
	int hopsize = 4;
	
	inputLength = 39;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_2Buffers){
	// Here we are testing sigOpt using 2 Buffers

	int numWindows = 7;
	int initialOffset = 6;
	int winSize = 16;
	int hopsize = 4;
	
	inputLength = 26;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_1CompleteBuffer){
	// Here we are testing sigOpt using 1 Complete Buffer

	// due to the large initial offset, including any more initial windows,
	// there will be only 1 index in the final window
	int numWindows = 3;
	int initialOffset = 6;
	int winSize = 16;
	int hopsize = 4;
	
	inputLength = 16;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_1PartialBuffer){
	// Here we are testing sigOpt using a stream that is long enough to
	// partially fill the 1st buffer

	// due to the large initial offset, including any more initial windows,
	// there will be only 1 index in the final window

	int numWindows = 3;
	int initialOffset = 6;
	// if numWindows is set to 4 and initialOffset is set to 1 then we get
	// a segmentation fault. For now I'm inclined to believe this is just
	// related to the winSize changing and us do something silly.
	int winSize = 16;
	int hopsize = 4;

	inputLength = 11;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_1MinimalBuffer){
	// Here we are testing sigOpt using a stream that is just long enough
	// to be used by sigOpt at all.

	// in the second window there are only 2 values (due to the value of
	// initialOffset - if initialOffset is decreased a larger number of
	// values will be included. If initialOffset is increased at all, then
	// the second window will contain fewer than 2 values and sigma will
	// not be calculated.)
	int numWindows = 2;
	int initialOffset = 5;
	int winSize = 16;
	int hopsize = 4;
	
	inputLength = 4;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_1HopSmallWin){
	// Here we are testing a case with 0 initial Offset
	int numWindows = 6;
	int initialOffset = 0;
	int winSize = 3;
	int hopsize = 1;
	
	inputLength = 6;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

START_TEST(test_sigOptFullRollSigma_3HopBigWin){
	// Here we are testing a case with 0 initial Offset, but a much larger
	// window
	int numWindows = 6;
	int initialOffset = 0;
	int winSize = 18;
	int hopsize = 2; // it works if we have a hopsize of 3

	inputLength = 15;
	sigOptFullRollSigma_testHelper(initialOffset, hopsize,
				       1.06, winSize, inputLength, numWindows,
				       fltInput);
} END_TEST

Suite *sigOpt_suite(void)
{
	Suite *s;
	TCase *tc_sOcore;
	TCase *tc_sOlimits;
	TCase *tc_sOrun;
	TCase *tc_bIcore;
	TCase *tc_bIlimits;
	TCase *tc_bIcomp;

	s = suite_create("sigOpt");

	/* Core test case */
	tc_sOcore = tcase_create("sigOpt Core");
	tcase_add_test(tc_sOcore,test_sigOptCreate);
	tcase_add_test(tc_sOcore,test_sigOptSetTerminationIndex);
	suite_add_tcase(s, tc_sOcore);

	tc_sOlimits = tcase_create("sigOpt Limits");
	// we could probably use some tests to ensure we avoid overflows from
	// input parameters
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negWinSize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroWinSize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negHopsize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroHopsize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negScaleFactor);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroScaleFactor);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negInitialOffset);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroInitialOffset);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_InitialOffsetWinSize);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_negChannel);
	tcase_add_test(tc_sOlimits,test_sigOptCreate_zeroChannel);
	suite_add_tcase(s, tc_sOlimits);

	tc_sOrun = tcase_create("sigOpt Run Examples");
	if (isLittleEndian() == 1){
		
		tcase_add_checked_fixture(tc_sOrun, setup_sigOptRunExamples,
					  teardown_sigOptRunExamples);
		/*
		tcase_add_test(tc_sOrun, test_sigOptFullRollSigma_Simple);
		tcase_add_test(tc_sOrun,
			       test_sigOptFullRollSigma_SimpleSmallerTermInd);
		tcase_add_test(tc_sOrun,
			       test_sigOptFullRollSigma_SimpleLargerOffset);
		tcase_add_test(tc_sOrun, test_sigOptFullRollSigma_Even);
		tcase_add_test(tc_sOrun,
			       test_sigOptFullRollSigma_EvenSmallStream);
		tcase_add_test(tc_sOrun,
			       test_sigOptFullRollSigma_EvenExactFill);
		tcase_add_test(tc_sOrun, test_sigOptFullRollSigma_3Buffers);
		tcase_add_test(tc_sOrun, test_sigOptFullRollSigma_2Buffers);
		
		tcase_add_test(tc_sOrun,
			       test_sigOptFullRollSigma_1CompleteBuffer);
		tcase_add_test(tc_sOrun,
			       test_sigOptFullRollSigma_1PartialBuffer);
		tcase_add_test(tc_sOrun,
			       test_sigOptFullRollSigma_1MinimalBuffer);
		tcase_add_test(tc_sOrun, test_sigOptFullRollSigma_1HopSmallWin);
		*/
		tcase_add_test(tc_sOrun, test_sigOptFullRollSigma_3HopBigWin);
		// could still use some tests on using different numbers
		// channels to ensure that there is no cases where
		// modifications to the information for one channel don't
		// impact a different channel
	} else {
		printf("Cannot add rollSigma tests because the tests are not \n"
		       "presently equipped to run on Big Endian machines\n");
	}
	suite_add_tcase(s,tc_sOrun);

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
