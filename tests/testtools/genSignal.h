#include <stddef.h> // size_t


/// Produces a signal that has a delta function at t=0 and is zero elsewhere
void genImpulse(void *array, enum PrecisionEnum precision, int length,
		double amplitude);

/// this initializes a staircase function
///
/// This function was defined as part of a misunderstanding of input the input
/// signal required to compute a step response
void genStaircaseFunction(void *array, enum PrecisionEnum precision,
			  int length, double start, double increment,
			  int steplength);

/// Generates a SineWave
void genSineWave(void *array, enum PrecisionEnum precision,
		 size_t length, double period_sec, size_t sample_rate_Hz,
		 double amplitude, double phase);

/// Generates a SquareWave 
void genSquareWave(void *array, enum PrecisionEnum precision,
		   size_t length, double period_sec, size_t sample_rate_Hz,
		   double amplitude, double phase);
