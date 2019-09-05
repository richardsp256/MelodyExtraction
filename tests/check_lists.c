#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <check.h>
#include "../src/lists.h"


// This should be broken down into more atomic tests
// (This was created using throwaway testing code)
START_TEST(test_orderedList)
{
	float abs_tol = 1.e-6;

	struct orderedList l = orderedListCreate(18);
	ck_assert_int_eq(l.length, 0);
	ck_assert_int_eq(l.capacity, 18);

	// insert elements
	orderedListInsert(&l, 63.4);
	orderedListInsert(&l, 1244.);
	orderedListInsert(&l, 542.3);
	orderedListInsert(&l, 17.8);

	ck_assert_int_eq(l.length, 4);
	ck_assert_float_eq_tol(l.array[0], 17.8, abs_tol);
	ck_assert_float_eq_tol(l.array[1], 63.4, abs_tol);
	ck_assert_float_eq_tol(l.array[2], 542.3, abs_tol);
	ck_assert_float_eq(l.array[3], 1244.);

	// try to delete some entries
	orderedListDeleteEntries(&l, 1, 3);

	ck_assert_int_eq(l.length, 2);
	ck_assert_float_eq_tol(l.array[0], 17.8, abs_tol);
	ck_assert_float_eq(l.array[1], 1244.);

	// add another element after deletion
	orderedListInsert(&l, 89.43);

	ck_assert_int_eq(l.length, 3);
	ck_assert_float_eq_tol(l.array[0], 17.8, abs_tol);
	ck_assert_float_eq_tol(l.array[1], 89.43, abs_tol);
	ck_assert_float_eq(l.array[2], 1244.);

	// now let's make sure that the ordering is correct if we insert
	// duplicate values
	orderedListInsert(&l, 17.8);
	orderedListInsert(&l, 89.43);

	ck_assert_int_eq(l.length, 5);
	ck_assert_float_eq_tol(l.array[0], 17.8, abs_tol);
	ck_assert_float_eq_tol(l.array[1], 17.8, abs_tol);
	ck_assert_float_eq_tol(l.array[2], 89.43, abs_tol);
	ck_assert_float_eq_tol(l.array[3], 89.43, abs_tol);
	ck_assert_float_eq(l.array[4], 1244.);

	// Let's make sure duplicate values get properly deleted
	orderedListDeleteEntries(&l, 0, 2);

	ck_assert_int_eq(l.length, 3);
	ck_assert_float_eq_tol(l.array[0], 89.43, abs_tol);
	ck_assert_float_eq_tol(l.array[1], 89.43, abs_tol);
	ck_assert_float_eq(l.array[2], 1244.);

	// Finally, let's make sure that we can delete all entries
	orderedListDeleteEntries(&l, 0, 3);

	ck_assert_int_eq(l.length, 0);

	orderedListDestroy(l);
}
END_TEST




Suite * orderedList_suite(void)
{
	Suite *s = suite_create("orderedList");
	// create general test case (should be split into smaller tests)
	TCase *tc_general = tcase_create("General");
	tcase_add_test(tc_general, test_orderedList);
	suite_add_tcase(s, tc_general);
	return s;
}

// This should be broken down into more atomic tests
// (This was created using throwaway testing code)
START_TEST(test_distinctList)
{
	float abs_tol = 1.e-6;

	distinctList *l = distinctListCreate(18);
	 
	ck_assert_int_eq(l->length, 0);
	ck_assert_int_eq(l->capacity, 18);

	// Try appending some values

	distinctListAppend(l, (struct distinctCandidate){190., 7, 0., -1});
	distinctListAppend(l, (struct distinctCandidate){ 96., 2, 0., -1});
	distinctListAppend(l, (struct distinctCandidate){121., 1, 0., -1});
	distinctListAppend(l, (struct distinctCandidate){242., 1, 0., -1});
	distinctListAppend(l, (struct distinctCandidate){391., 1, 0., -1});

	ck_assert_int_eq(l->length, 5);
	ck_assert_int_eq(l->capacity, 18);

	// check the values of the structs in the list
	ck_assert_float_eq(l->array[0].frequency, 190.);
	ck_assert_float_eq(l->array[1].frequency,  96.);
	ck_assert_float_eq(l->array[2].frequency, 121.);
	ck_assert_float_eq(l->array[3].frequency, 242.);
	ck_assert_float_eq(l->array[4].frequency, 391.);

	ck_assert_int_eq(l->array[0].confidence, 7);
	ck_assert_int_eq(l->array[1].confidence, 2);
	ck_assert_int_eq(l->array[2].confidence, 1);
	ck_assert_int_eq(l->array[3].confidence, 1);
	ck_assert_int_eq(l->array[4].confidence, 1);


	for (int i = 0; i<5; i++){
		ck_assert_float_eq(l->array[i].cost, 0.f);
		ck_assert_int_eq(l->array[i].indexLowestCost, -1);
	}

	// try to shrink capacity
	distinctListShrink(l);
	ck_assert_int_eq(l->length, 5);
	ck_assert_int_eq(l->capacity, 5);


	// Test the utility used to alter the cost of a distinctCandidate
	distinctListAdjustCost(l, 3, 5.5f, 2);

	for (int i = 0; i<5; i++){
		if (i != 3){
			ck_assert_float_eq(l->array[i].cost, 0.f);
			ck_assert_int_eq(l->array[i].indexLowestCost, -1);
		} else {
			ck_assert_float_eq(l->array[i].cost, 5.5f);
			ck_assert_int_eq(l->array[i].indexLowestCost, 2);
		}
	}

	distinctListDestroy(l);
}
END_TEST

Suite * distinctList_suite(void)
{
	Suite *s = suite_create("distinctList");
	// create general test case (should be split into smaller tests)
	TCase *tc_general = tcase_create("General");
	tcase_add_test(tc_general, test_distinctList);
	suite_add_tcase(s, tc_general);
	return s;
}


int main(void){

	SRunner *sr = srunner_create(orderedList_suite());
	srunner_add_suite(sr,distinctList_suite());

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	if (number_failed == 0){
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
