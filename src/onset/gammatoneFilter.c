#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include "../utils.h" // HasOverlap
#include "../resample.h"
#include "../errors.h"
#include "gammatoneFilter.h"

#if defined (__GNUC__) || defined (__clang__)
#define forceinline __attribute__((always_inline))
#else
#define forceinline /* ... */
#endif

/// numerically normalize the response to have 0 dB gain at the central
/// frequency.
///
/// Right now, I am just dividing the coefficients in the numerator
/// of each second order stage by the gain. This can almost certainly
/// be improved
///
/// We evaluate the transfer function of the biquad filter designated
/// by the 6 entries in coef.
/// The gain is the magnitude of the transfer function evaluated when
/// z = exp(I* 2*PI*centralFreq/samplerate)
static void numericalNormalize_(float centralFreq, int samplerate,
				double * coef)
{
	double x1, x2, gain;
	x1 = 2. * M_PI * centralFreq/samplerate;
	x2 = 4. * M_PI * centralFreq/samplerate;

	gain = sqrt((pow(coef[2]+coef[1]*cos(x1) + coef[0]* cos(x2),2.)
                 + pow(coef[1]*sin(x1) + coef[0] * sin(x2),2.))
                /( pow(coef[5]+coef[4]*cos(x1) + coef[3]* cos(x2),2.)
                   + pow(coef[4]*sin(x1) + coef[3] * sin(x2),2.)));
	coef[0] = coef[0]/gain;
	coef[1] = coef[1]/gain;
	coef[2] = coef[2]/gain;
}

void sosGammatoneCoef(float centralFreq, int samplerate, double *coef)
{
	double delta_t = 1./(double)samplerate;
	double cf = (double)centralFreq;
	double b = 2*M_PI*1.019*24.7*(4.37*cf/1000. + 1); //bandwidth

	// Now to actually set the coefficients for each stae of filtering.
	// The only coefficient that changes between stages is b1
	for (int i = 0; i < 4; i++){
		// We  start by setting b0
		coef[6*i] = delta_t;
		// set b1
		if (i<2){
			coef[6*i+1] = (-((2 * delta_t * cos(2 * cf * M_PI * delta_t)
					  / exp(b * delta_t))
					 + (pow(-1,(double)i) * 2
					    * sqrt(3 + pow(2., 1.5)) * delta_t *
					    sin(2 * cf * M_PI * delta_t)
					    / exp(b * delta_t))) / 2.);
		} else {
			coef[6*i+1] = -(2 * delta_t * cos(2 * cf * M_PI *
							  delta_t)
					/ exp(b * delta_t)
					+ pow(-1,(double)i) * 2
					* sqrt(3 - pow(2.,1.5)) * delta_t *
					sin(2. * cf * M_PI * delta_t)
					/ exp(b * delta_t)) / 2.;
		}		
		// set b2
		coef[6*i+2] = 0;
		// set a0
		coef[6*i+3] = 1;
		// set a1
		coef[6*i+4] = -2*cos(2*cf*M_PI*delta_t)/exp(b*delta_t);
		// set a2
		coef[6*i+5] = exp(-2*b*delta_t);

		// now to numerically normalize
		numericalNormalize_(centralFreq, samplerate, coef+6*i);
	}
}


static forceinline double sosFilterElem_(
	double x, const double * restrict coef,
	uint8_t n_stages, double * restrict state)
{
	// implementation commonly require that a0 = 1 (to avoid the division)
	double new_val = (double)x;
	for (uint8_t stage = 0; stage < n_stages; stage++){
		double cur_x = new_val;

		// load feedforward coefficients (coefficients in numerator of
		// the transfer function)
		double b0, b1, b2;
		b0 = coef[stage * 6 + 0];
		b1 = coef[stage * 6 + 1];
		b2 = coef[stage * 6 + 2];

		// load feedback coefficients (coefficients in denominator of
		// the transfer function)
		double a0, a1, a2;
		a0 = coef[stage * 6 + 3];
		a1 = coef[stage * 6 + 4];
		a2 = coef[stage * 6 + 5];

		// d1 and d2 are state variables
		int d1_ind = 2*stage;
		int d2_ind = 2*stage + 1;

		// 1st eqn: y[n] = (b0 * x[n] + d1[n-1])/a0
		new_val = (b0 * cur_x + state[d1_ind])/a0;
		// 2nd eqn: d1[n] = b1 * x[n] - a1 * y[n] + d2[n-1]
		state[d1_ind] =
			b1 * cur_x - a1 * new_val + state[d2_ind];
		// 3rd eqn: d2[n] = b2 * x[n] - a2 * y[n]
		state[d2_ind] = b2 * cur_x - a2 * new_val;
	}
	return new_val;
}

static void sosFilter_(const double * restrict x, size_t length,
		       const double * restrict coef, uint8_t n_stages,
		       double * restrict y, double * restrict state)
{
	for (size_t n = 0; n < length; n++){
		y[n] = (double) sosFilterElem_( (double)x[n], coef,
						n_stages, state);
	}
}

int sosFilter(int num_stages, const double *coef, const double *x, double *y,
	      int length)
{
	if (num_stages > 8){
		abort();
	} else if (num_stages <= 0){
		return ME_ERROR;
	} else if (HasOverlap(x, length, y, length, sizeof(double)) ||
		   HasOverlap(x, length, coef, num_stages*6, sizeof(double)) ||
		   HasOverlap(y, length, coef, num_stages*6, sizeof(double))){
		return ME_ERROR;
	}
	double state[16] = {0.}; // automatically initialized to 0s
	sosFilter_(x, length, coef, (uint8_t) num_stages,  y, state);
	return ME_SUCCESS;
}

int biquadFilter(const double *coef, const double *x, double *y, int length){
	double state[2] = {0.,0.};
	return sosFilter(1, coef, x, y, length);
}

static void sosFilterf_(const float * restrict x, size_t length,
			const double * restrict coef, uint8_t n_stages,
			float * restrict y, double * restrict state)
{
	for (size_t n = 0; n < length; n++){
		y[n] = (float) sosFilterElem_( (double)x[n], coef,
					       n_stages, state);
	}
}

int sosGammatone(const double* data, double* output, float centralFreq,
		 int samplerate, int datalen)
{
	double coef[24];
	sosGammatoneCoef(centralFreq, samplerate, coef);

	// for now, we asssume that state variables start at 0, because before
	// a recording there is silence.
	return sosFilter(4, coef, data, output, datalen);
}

int sosGammatonef(const float* data, float* output, float centralFreq,
		  int samplerate, int datalen)
{
	if ( HasOverlap(data, datalen, output, datalen, sizeof(float)) ){
		return ME_ERROR;
	}
	double coef[24];
	sosGammatoneCoef(centralFreq, samplerate, coef);

	// for now, we asssume that state variables start at 0, because before
	// a recording there is silence. If we are chunking the recording we
	// will need to track state between chunks.
	double state[8] = {0.}; // automatically initialized to 0s
	sosFilterf_(data, datalen, coef, 4, output, state);
	return ME_SUCCESS;
}
