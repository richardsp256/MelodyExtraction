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
	four_channel_fB = filterBankNew(4, 47, 9, 11025, 80, 4000);
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

Suite *filterBank_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("filterBank");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_add_checked_fixture(tc_core, setup_emptyFourChannelfB,
				  teardown_FourChannelfB);
	tcase_add_test(tc_core,test_filterBankCreate);
	tcase_add_test(tc_core,test_filterBankSimpleInput);

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
