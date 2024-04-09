#include <math.h> // sin, M_PI
#include "testtools.h"

void genImpulse(void *array, enum PrecisionEnum precision, int length,
		double amplitude)
{
	switch(precision){
	case float_precision:
		((float*)array)[0] = amplitude;
		for (int i = 1; i < length; i++){
			((float*)array)[i] = 0;
		}
		break;
	case double_precision:
		((double*)array)[0] = amplitude;
		for (int i = 1; i < length; i++){
			((double*)array)[i] = 0;
		}
		break;
	}
}

void genStaircaseFunction(void *array, enum PrecisionEnum precision,
			  int length, double start, double increment,
			  int steplength){
	double cur = start;
	int j=0;
	for (int i = 0; i <length; i++){
		if (i >= j*steplength){
			j++;
			cur = increment*((double)j)+start;
		}
		switch(precision){
		case float_precision:
			((float  *)array)[i] = (float) cur;
			break;
		case double_precision:
			((double *)array)[i] = (double)cur;
			break;
		}
	}
}

void genSineWave(void *array, enum PrecisionEnum precision,
		 size_t length, double period_sec, size_t sample_rate_Hz,
		 double amplitude, double phase)
{

	double k = 2.*M_PI/(period_sec*(double)sample_rate_Hz);
	for (size_t i = 0; i <length; i++){
		double val = amplitude*sin(k * (double)i + phase);
		switch(precision){
		case float_precision:
			((float  *)array)[i] = (float) val;
			break;
		case double_precision:
			((double *)array)[i] = (double)val;
			break;
		}
	}
}


// we can implement this more cheaply (without trigonometric functions)
void genSquareWave(void *array, enum PrecisionEnum precision,
		   size_t length, double period_sec, size_t sample_rate_Hz,
		   double amplitude, double phase)
{
	double k = 2.*M_PI/(period_sec*(double)sample_rate_Hz);
	for (size_t i = 0; i <length; i++){
		double tmp = sin(k * (double)i + phase);
		// val = amplitude*sign(tmp)
		// - if tmp  > 0, val = amplitude
		// - if tmp == 0, val = 0
		// - if tmp  < 0, val = -amplitude
		double val = amplitude*((tmp > 0) - (tmp < 0));
		switch(precision){
		case float_precision:
			((float  *)array)[i] = (float) val;
			break;
		case double_precision:
			((double *)array)[i] = (double)val;
			break;
		}
	}
}
