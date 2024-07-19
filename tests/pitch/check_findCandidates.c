#include <math.h> // fabsf
#include <check.h>

#include "../../src/lists.h"
#include "../../src/pitch/findCandidates.h"


// This is example is taken from the paper
START_TEST(RatioAnalysisCandidates_PaperExample)
{
	// This test uses the example from figure 2 of the BaNa paper
	const float peakFreq[5] = {192., 391., 485., 581., 760.};
	const int numPeaks = 5;

	struct orderedList candidates =
		orderedListCreate( (numPeaks * (numPeaks-1))/2 + 2);
	RatioAnalysisCandidates(peakFreq, numPeaks, &candidates);

	ck_assert_int_eq(candidates.length, 10);

	float expected[10] = {96., 98., 121.,
			      192., 192., 192., 194., 196.,
			      242., 391.};
	for (int i = 0; i < 10; i++){
		float diff = orderedListGet(candidates, i) - expected[i];
		ck_assert_msg((fabsf(diff)<=0.5f),
			      "Expected %6.2f at %d. Got %6.2f, instead",
			      expected[i], i, orderedListGet(candidates, i));
	}

	orderedListDestroy(candidates);
}


START_TEST(distinctCandidates_PaperExample)
{
	// This test uses the example from figure 2 of the BaNa paper
	float input_candidate_l[10] = {96., 98., 121.,
				       192., 192., 192., 194., 196.,
				       242., 391.};

	struct orderedList candidates =	orderedListCreate(12);

	for (int i = 0; i < 10; i++){
		orderedListInsert(&candidates, input_candidate_l[i]);
	}

	orderedListInsert(&candidates, 190.); // cepstrum frequency
	orderedListInsert(&candidates, 192.); // lowest frequency peak

	distinctList * distinct = distinctCandidates(&candidates, 12, 10.,
						     50., 600.);

	// orderedList should now be empty
	ck_assert_int_eq(candidates.length, 0);
	orderedListDestroy(candidates);

	float expected_candidates[5] = {190., 96., 121., 242., 391};
	int   expected_confidence[5] = {   7,   2,    1,    1,   1};
	ck_assert_int_eq(distinct->length, 5);
	for (int i = 0; i < 5; i++){
		ck_assert_float_eq(distinctListGet(distinct, i).frequency,
				   expected_candidates[i]);
		ck_assert_int_eq(distinctListGet(distinct, i).confidence,
				 expected_confidence[i]);
	}
	distinctListDestroy(distinct);
}

Suite * findCandidates_suite(void)
{
	Suite *s = suite_create("findCandidates");
	TCase *tc_ratio_candidates = tcase_create("RatioAnalysisCandidates");
	tcase_add_test(tc_ratio_candidates,
		       RatioAnalysisCandidates_PaperExample);

	TCase *tc_distinct_candidates = tcase_create("distinctCandidates");
	tcase_add_test(tc_distinct_candidates,
		       distinctCandidates_PaperExample);

	suite_add_tcase(s, tc_ratio_candidates);
	suite_add_tcase(s, tc_distinct_candidates);
	return s;
}
