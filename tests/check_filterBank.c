#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <limits.h>
#include "doubleArrayTesting.h"
#include "../src/tripleBuffer.h"
#include "../src/filterBank.h"


/* This set of tests includes only a very simplistic test of basic 
 * functionallity.
 *
 * This set of tests is not designed to be as exhaustive as those for sigOpt or 
 * tripleBuffer. (This is mainly because the functionallity is simpler, and we 
 * should be probably be mocking interactions with tripleBuffer)
 */

filterBank *four_channel_fB;

void setup_emptyFourChannelfB(void){
	four_channel_fB = filterBankNew(4, 47, 9, 11025, 80, 4000,"default");
}

void teardown_FourChannelfB(void){
	filterBankDestroy(four_channel_fB);
}

START_TEST(test_filterBankCreate)
{
	ck_assert_ptr_nonnull(four_channel_fB);
	ck_assert_int_eq(filterBankFirstChunkLength(four_channel_fB), 47);
	ck_assert_int_eq(filterBankNormalChunkLength(four_channel_fB), 38);
}
END_TEST

START_TEST(test_filterBankSimpleInput)
{
	float* chunk = malloc(sizeof(float)*47);
	for (int i=0; i<47; i++){
		chunk[i] = 0.f;
	}
	chunk[0] = 1;

	int result = filterBankSetInputChunk(four_channel_fB, chunk, 47, 0);
	ck_assert_int_eq(result,1);

	tripleBuffer *tB = tripleBufferCreate(4,47);
	tripleBufferAddLeadingBuffer(tB);
	result = filterBankProcessInput(four_channel_fB, tB, 0);
	ck_assert_int_eq(result,1);

	float cf = filterBankCentralFreq(four_channel_fB, 0);
	printf("CENTRAL FREQ = %f\n",cf);

	double *result_array =NULL;
	float_to_double_array(tripleBufferGetBufferPtr(tB, 0, 0), 47,
			      &result_array);
	tripleBufferDestroy(tB);

	float* ref = malloc(sizeof(float)*47);
	sosGammatoneFast(chunk,&ref,cf, 11025,47);
	double* reference = NULL;
	float_to_double_array(ref, 47, &reference);
	free(ref);
	free(chunk);

	// The reduced error is a consequence of the difference in how we
	// perform the Gammatone calculation
	compareArrayEntries(reference, result_array, 47,
			    6.e-4, 1, 6.e-4);

	free(reference);
	free(result_array);
}
END_TEST

/* Here we are implementing a slightly different variation of sosGammatone 
 * filter to perfectly match the implementation in filterBank.
 */


/* This next function goes through and computes the results for filterBank if 
 * funcNum is 1 or using the gammatoneFilter if funcNum is 0. */
double* filterInputs(int numChannels, int lenChannels, int overlap,
		     int samplerate, float minFreq, float maxFreq,
		     float* input, int dataLen, int funcNum){
	
	
	if (funcNum==0) {
		float* centralFreq = centralFreqMapper(numChannels, minFreq,
						       maxFreq);
		int result_length = numChannels*dataLen;
		float* fltResultA = malloc(sizeof(float) * result_length);

		for (int i = 0; i<numChannels;i++){
			float* start = fltResultA; //+ i*dataLen;
			
			sosGammatoneFast(input, &start, centralFreq[i],
					 samplerate, dataLen);
		}

		double *returnArray = malloc(sizeof(double)*
					     (numChannels*dataLen));
		for (int j =0; j<dataLen*numChannels;j++){
			returnArray[j] = (double)fltResultA[j];
		}
		free(fltResultA);
		free(centralFreq);
		return returnArray;
	}else{
		double *returnArray = NULL;
		float *fltResult = fullFiltering(numChannels, lenChannels,
						 overlap, samplerate, minFreq,
						 maxFreq, input, dataLen);
		float_to_double_array(fltResult, dataLen*numChannels,
				      &returnArray);
		free(fltResult);
		return returnArray;
	}
}

START_TEST(test_filterBank_2Chunks)
{
	// Lets try the case where the input can fit into 2 chunks
	int numChannels = 4;
	int lenChannels = 50;
	int overlap = 10;
	int samplerate = 11025;
	int minFreq = 80;
	int maxFreq = 4000;
	int dataLen = 75;
	
	float* input = malloc(sizeof(float)*dataLen);
	for (int i=0; i<dataLen; i++){
		input[i] = 0.f;
	}
	input[0] = 1;
	input[1] = 0.5;
	input[lenChannels] = -0.3;

	double* reference = filterInputs(numChannels, lenChannels, overlap,
					 samplerate, minFreq, maxFreq, input,
					 dataLen, 0);
	ck_assert_ptr_nonnull(reference);
	double* result = filterInputs(numChannels, lenChannels, overlap,
				      samplerate, minFreq, maxFreq, input,
				      //dataLen, 1);
				      47,1);
	ck_assert_ptr_nonnull(result);

	compareArrayEntries(reference, result, dataLen, 1.e-5, 1, 1.e-5);

	free(input);
	free(result);
	free(reference);
}
END_TEST

Suite *filterBank_suite(void)
{
	Suite *s;
	TCase *tc_core;
	TCase *tc_run;
	s = suite_create("filterBank");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_add_checked_fixture(tc_core, setup_emptyFourChannelfB,
				  teardown_FourChannelfB);
	tcase_add_test(tc_core,test_filterBankCreate);
	tcase_add_test(tc_core,test_filterBankSimpleInput);

	suite_add_tcase(s,tc_core);

	tc_run = tcase_create("filterBank Run Examples");
	tcase_add_test(tc_run,test_filterBank_2Chunks);

	suite_add_tcase(s,tc_run);
	
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
