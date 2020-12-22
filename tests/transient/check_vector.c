#ifdef USE_SSE_INTRINSICS
#include "../../src/transient/vector_sse.h"
#else
#include "../../src/transient/vector_scalar.h"
#endif

#include "../../src/utils.h" // AlignedAlloc
#include <stdio.h> // printf
#include <check.h>

// For each test problem, we setup the following set of global variables
static float * arr_in0; // initialized to an array holding {0, 1, 2, 3}
static float * arr_in1; // initialized to an array holding {4, 5, 6, 7}
static float * arr_out; // initialized to an array holding {-1, -1, -1, -1}

void setup(void)
{
	arr_in0 = AlignedAlloc(16,4*sizeof(float));
	for (int i = 0; i < 4; i++) { arr_in0[i] = (float)i; }

	arr_in1 = AlignedAlloc(16,4*sizeof(float));
	for (int i = 0; i < 4; i++) { arr_in1[i] = 4. + (float)i; }

	arr_out = AlignedAlloc(16,4*sizeof(float));
	for (int i = 0; i < 4; i++) { arr_out[i] = -1.f; }
}

void teardown(void)
{
	free(arr_in0);
	arr_in0 = NULL;

	free(arr_in1);
	arr_in1 = NULL;

	free(arr_out);
	arr_out = NULL;
}

START_TEST(test_load_store_f32x4)
{
	f32x4 vector = load_f32x4(arr_in0);
	store_f32x4(arr_out, vector);

	for (int i=0; i<4; i++){
		ck_assert_float_eq(arr_out[i], arr_in0[i]);
	}
}
END_TEST

START_TEST(test_broadcast_scalar_f32x4)
{
	f32x4 vector = broadcast_scalar_f32x4(0.);
	store_f32x4(arr_out, vector);

	for (int i=0; i<4; i++){
		ck_assert_float_eq(arr_out[i], 0.f);
	}
}
END_TEST

START_TEST(test_add_f32x4)
{
	f32x4 v1 = load_f32x4(arr_in0);
	f32x4 v2 = load_f32x4(arr_in1);
	store_f32x4(arr_out, add_f32x4(v1, v2));

	ck_assert_float_eq(arr_out[0], 4.f);
	ck_assert_float_eq(arr_out[1], 6.f);
	ck_assert_float_eq(arr_out[2], 8.f);
	ck_assert_float_eq(arr_out[3], 10.f);
}
END_TEST

START_TEST(test_sub_f32x4)
{
	f32x4 v1 = load_f32x4(arr_in0);
	f32x4 v2 = load_f32x4(arr_in1);
	store_f32x4(arr_out, sub_f32x4(v1, v2));

	for (int i=0; i<4; i++){
		ck_assert_float_eq(arr_out[i], -4.f);
	}
}
END_TEST

START_TEST(test_mul_f32x4)
{
	f32x4 v1 = load_f32x4(arr_in0);
	f32x4 v2 = load_f32x4(arr_in1);
	store_f32x4(arr_out, mul_f32x4(v1, v2));

	ck_assert_float_eq(arr_out[0], 0.f);
	ck_assert_float_eq(arr_out[1], 5.f);
	ck_assert_float_eq(arr_out[2], 12.f);
	ck_assert_float_eq(arr_out[3], 21.f);
}
END_TEST

START_TEST(test_lshift_extract_f32x4)
{
	f32x4 v1 = load_f32x4(arr_in0);
	f32x4 v2 = load_f32x4(arr_in1);

	store_f32x4(arr_out, lshift_extract_f32x4(v1, v2, 0 ));
	for (int i=0; i<4; i++){
		ck_assert_float_eq(arr_out[i], arr_in0[i]);
	}

	float expected1[4] = {1., 2., 3., 4.};
	store_f32x4(arr_out, lshift_extract_f32x4(v1, v2, 1 ));
	for (int i=0; i<4; i++){
		ck_assert_float_eq(arr_out[i], expected1[i]);
	}

	float expected2[4] = {2., 3., 4., 5.};
	store_f32x4(arr_out, lshift_extract_f32x4(v1, v2, 2 ));
	for (int i=0; i<4; i++){
		ck_assert_float_eq(arr_out[i], expected2[i]);
	}

	float expected3[4] = {3., 4., 5., 6.};
	store_f32x4(arr_out, lshift_extract_f32x4(v1, v2, 3 ));
	for (int i=0; i<4; i++){
		ck_assert_float_eq(arr_out[i], expected3[i]);
	}
}
END_TEST

Suite * f32x4_suite(void)
{
	printf("Testing the \"%s\" vector implementation\n",
	       vector_backend_name());
	Suite *s = suite_create("f32x4");
	// create general test case (should be split into smaller tests)
	TCase *tc_general = tcase_create("General");
	tcase_add_checked_fixture(tc_general, setup, teardown);

	tcase_add_test(tc_general, test_load_store_f32x4);
	tcase_add_test(tc_general, test_broadcast_scalar_f32x4);
	tcase_add_test(tc_general, test_add_f32x4);
	tcase_add_test(tc_general, test_sub_f32x4);
	tcase_add_test(tc_general, test_mul_f32x4);
	tcase_add_test(tc_general, test_lshift_extract_f32x4);

	suite_add_tcase(s, tc_general);
	return s;
}
