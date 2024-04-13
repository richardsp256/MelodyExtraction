/// @file     errors.h
/// @brief    Declaration of error reporting utilities
///
/// To simplify the implementation/maintenance of error codes, they are
/// implemented with X-Macros. The errors are defined in errors.def

#ifndef ERRORS_H
#define ERRORS_H
enum me_errors{
#define ERROR_ENTRY(name, val, str) name = val,
	#include "errors.def"
#undef ERROR_ENTRY
};

#endif /* ERRORS_H */

// note: me_strerror is declared within melodyextraction.h so that we don't
//       need to worry about X-Macro issues when including the library in
//       external applications.


