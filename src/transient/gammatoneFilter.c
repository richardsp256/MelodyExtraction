/// @file     gammatoneFilter.c
/// @brief    Implementation of the gammatone filter.
///
/// It may make more sense to place the sosFilter in a separate file in a
/// common module in the root file, so that we can recycle it

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

static void sosFilter_(const float * restrict x, size_t length,
		       const double * restrict coef, uint8_t n_stages,
		       float * restrict y, double * restrict state)
{
	for (size_t n = 0; n < length; n++){
		// implementations commonly require that a0 = 1 (to avoid the
		// single division)
		double new_val = (double)x[n];
		for (uint8_t stage = 0; stage < n_stages; stage++){
			double cur_x = new_val;

			// load feedforward coefficients (coefficients in
			// numerator of the transfer function)
			double b0, b1, b2;
			b0 = coef[stage * 6 + 0];
			b1 = coef[stage * 6 + 1];
			b2 = coef[stage * 6 + 2];

			// load feedback coefficients (coefficients in
			// denominator of the transfer function)
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
		y[n] = (float) new_val;
	}
}

int sosFilter(int num_stages, const double *coef, const float *x, float *y,
	       int length)
{
	if (num_stages > 8){
		return ME_SOSFILTER_TOO_MANY_STAGES;
	} else if (num_stages <= 0){
		return ME_SOSFILTER_ZERO_STAGES;
	} else if (HasOverlap(x, length, y, length, sizeof(float))){
		return ME_SOSFILTER_OVERLAPPING_ARRAYS;
	}
	double state[16] = {0.}; // automatically initialized to 0s
	sosFilter_(x, length, coef, (uint8_t) num_stages,  y, state);
	return ME_SUCCESS;
}

int sosGammatone(const float* data, float* output, float centralFreq,
		 int samplerate, int datalen)
{
	double coef[24];
	sosGammatoneCoef(centralFreq, samplerate, coef);

	// for now, we asssume that state variables start at 0, because before
	// a recording there is silence. If we are chunking the recording we
	// will need to track state between chunks.
	double state[8] = {0.}; // automatically initialized to 0s
	return sosFilter(4, coef, data, output, datalen);
}

void centralFreqMapper(size_t numChannels, float minFreq, float maxFreq,
		       float* fcArray){

	if (numChannels == 0){
		return;
	} else if (numChannels == 1){
		fcArray[0] = minFreq;
		return;
	}

	// calculate the minimum central frequency
	fcArray[0] = (minFreq + 12.35)/0.9460305;

	// calculate the maximum central frequency
	fcArray[numChannels-1] = (maxFreq - 12.35)/1.0539695;

	// calculate minERBS and maxERBS
	float minERBS = 21.3 * log10f(1+0.00437*fcArray[0]);
	float maxERBS = 21.3 * log10f(1+0.00437*fcArray[numChannels-1]);

	// calculate all other entries of fcArray
	for (size_t i = 1; i < numChannels - 1; i++){
		fcArray[i] = ((powf(10.,((minERBS + (float)i * (maxERBS-minERBS)
					  /((float)numChannels - 1))/21.4))
			       - 1.0) /0.00437);
	}
}
