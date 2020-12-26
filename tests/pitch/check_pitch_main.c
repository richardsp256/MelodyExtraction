#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <check.h>
#include "check_pitch.h"

int main(void){
	SRunner *sr = srunner_create(findCandidates_suite());

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	if (number_failed == 0){
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
