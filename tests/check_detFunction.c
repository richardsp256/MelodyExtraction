#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../src/simpleDetFunc.h"
#include "doubleArrayTesting.h"

int rollSigmaTestTemplate(int *intInput, int intInputLen, double *dblInput,
			  int dblInputLen, char *strInput, double **array){
	/* this is the template for testing rollSigma.

	   There is asumed to be 4 intInput arguments and 1 dblInput argument.
	   strInput is assumed to be the file name from which the input is 
	   loaded.

	   The result must be converted from a float array to a double array.
	 */
	ck_assert_int_eq(intInputLen,4);
	ck_assert_int_eq(dblInputLen,1);

	int numWindows = intInput[3];

	/* load the input array. */

	int input_length;
	double *original_input;

	int little_endian = isLittleEndian();

	input_length = readDoubleArray(strInput, little_endian,
				       &original_input);

	/* convert the input array to an array of floats */
	float *buffer;
	double_to_float_array(original_input, input_length, &buffer);

	free(original_input);

	/* run the rollSigma function */
	float *original_sigma = malloc(sizeof(float)*numWindows);

	rollSigma(intInput[0], intInput[1], dblInput[0], intInput[2],
		  input_length, numWindows, buffer, &original_sigma);
	free(buffer);

	float_to_double_array(original_sigma, numWindows, array);

	free(original_sigma);

	return numWindows;
}

static struct dblArrayTestEntry *sigma_table;
static int sigma_table_length;



struct dblArrayTestEntry* construct_rollSigma_test_table()
{
	/* I am unsure of whether or not an unchecked fixture is called before 
	 * every loop in a loop test, or if it's called once at the start. 
	 * If the former case is true, then this is better off as an unchecked
	 * test fixture.
	 */
	sigma_table_length = 1;
	sigma_table = malloc(sizeof(struct dblArrayTestEntry)
			     * sigma_table_length);

	/* Setup the sample test.
	 */

	int intInput[] = {0,5,13,10};
	double dblInput[] = {2.0};
	
	//By listing path names from tests folder, the path will be correct
	//when we run make test from th emain folder, but incorrect when we 
	//run ./check_detFunction from the tests folder. If we list path 
	//names from test_files folder, same problem but reversed...
	setup_dblArrayTestEntry((sigma_table+0),intInput, 4, dblInput, 1,
				"tests/test_files/roll_sigma/roll_sigma_sample_input",
				"tests/test_files/roll_sigma/roll_sigma_sample_test",
				&rollSigmaTestTemplate);
	return sigma_table;
}

void destroy_roll_Sigma_test_table(){
	for (int i =0; i<sigma_table_length; i++){
		clean_up_dblArrayTestEntry(sigma_table + i);
	}
	free(sigma_table);
}

START_TEST (check_roll_sigma_table)
{
  ck_assert (process_double_array_test(sigma_table[_i], 1.e-5, 1, 1.e-5));
}
END_TEST

Suite *detFunction_suite()
{
	Suite *s = suite_create("detFunction");
	TCase *tc_rollSigma = tcase_create("rollSigma");

	if (isLittleEndian() == 1){
		tcase_add_loop_test(tc_rollSigma,check_roll_sigma_table,0,1);
	} else {
		printf("Cannot add rollSigma tests because the tests are not \n"
		       "presently equipped to run on Big Endian machines\n");
	}
	suite_add_tcase(s, tc_rollSigma);
	return s;
}

int main(void){
	construct_rollSigma_test_table();
	Suite *s = detFunction_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	destroy_roll_Sigma_test_table();
	if (number_failed == 0){
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
