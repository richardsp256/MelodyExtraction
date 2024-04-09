/// @file     errors.c
/// @brief    Implementation of error decoder
///
/// To simplify the implementation/maintenance of error codes, these are
/// implemented with X-Macros. The errors are defined in errors.def

#include <stddef.h> // declaration of NULL
#include "errors.h" // declaration of me_errors
// note: me_strerror is declared within melodyextraction.h so that we don't
//       need to worry about X-Macro issues when including the library in
//       external applications.

const char* me_strerror(int err_code){
	const char* out = NULL;

	switch(err_code){
#define ERROR_ENTRY(name, val, str) case name: { out = str; break; }
	#include "errors.def"
#undef ERROR_ENTRY
	}
	return out;
}
