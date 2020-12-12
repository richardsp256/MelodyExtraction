#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "../resample.h"



/* We are implementing the biquad filter with a Direct Form II Transposed 
 * Structure (this structure seems to be fairly standard for implementing 
 * digital biquad filters)
 *
 * coef: is a 6 entry array with the feedforward and feedback coefficients 
 *       coef[0]= b0, coef[1] = b1, coef[2] = b2
 *       coef[3]= a0, coef[4] = a1, coef[5] = a2
 * x: the input passed through the filter.
 * y: the output, it must already be allocated and have the same length as x. 
 *    To update the input array in place with the filter response, pass the 
 *    same argument for x into y. 
 * length: the length of both x and y
 * 
 * The difference equations are:
 *  y[n] = (b0 * x[n] + d1[n-1])/a0  
 *  d1[n] = b1 * x[n] - a1 * y[n] + d2[n-1]   
 *  d2[n] = b2 * x[n] - a2 * y[n]
 *
 * for now we are implementing this using doubles because precision matters.
 *
 * optimization: we could imitate many other implementations and require that
 *               a0 = 1.
 *
 * Probably need to be check for overflows and underflows - since this is 
 * recursive it could screw up a lot.
 */
void biquadFilter(double *coef, const double *x, double *y, int length)
{
	double d1, d2, cur_x, cur_y, a0,a1,a2,b0,b1,b2;
	int n;
	/* d1 and d2 are state variables, for now asssume they start at 0,
	 * because before a recording there is silence. If we are chunking the 
	 * recording we will need to track d1 and d2 between chunks. */
	d1 = 0;
	d2 = 0;

	/* set the feedforward coefficients */
	b0 = coef[0]; b1 = coef[1]; b2 = coef[2];
	/* set the feedback coefficients */
	a0 = coef[3]; a1 = coef[4]; a2 = coef[5];

	for (n = 0; n<length; n++){
		cur_x = x[n];
		/* first difference equation: y[n] = (b0 * x[n] + d1[n-1])/a0 
		 */
		cur_y = (b0 * cur_x + d1)/a0;

		/* next difference equation: 
		 *     d1[n] = b1 * x[n] - a1 * y[n] + d2[n-1]
		 */
		d1 = b1 * cur_x - a1 * cur_y + d2;
		/* final difference equation: d2[n] = b2 * x[n] - a2 * y[n]
		 */
		d2 = b2 * cur_x - a2 * cur_y;

		/* finally set y to cur_y */
		y[n] = cur_y;
	}
}

/* num_stages : the number of biquad filters to apply. Must be at least 1
 * coef: a num_stages * 6 entry array with the feedforward and feedback 
 *       coefficients for every stage of filtering. The coefficients of the ith
 *       stage should be at:
 *           coef[(i*6)+0]= b0, coef[(i*6)+1] = b1, coef[(i*6)+2] = b2
 *           coef[(i*6)+3]= a0, coef[(i*6)+4] = a1, coef[(i*6)+5] = a2
 * x: the input passed through the filter.
 * y: the output, it must already be allocated and have the same length as x. 
 *    To update the input array in place with the filter response, pass the 
 *    same argument for x into y.
 * length: the length of both x and y
 *
 * This can be optimized by doing all of the filtering for x[n] in one go (we 
 * would be doing the same number of additive/multiplicative operations (to 
 * maintain numerical stability), but it would cut down on the number of for 
 * loops (and the number of times we access the data)
 *
 * again, if processing audio in chunks, we will need to keep track of the 
 * state variables between function calls.
 */
void cascadeBiquad(int num_stages, double *coef, const double *x, double *y,
		   int length)
{
	/* run the filter the first time to produce y */
	biquadFilter(coef, x, y, length);

	/* run the biquad filter num_stages-1 more times, updating y in place 
	 */
	for (int i= 1; i<num_stages; i++){
		biquadFilter((coef + (6*i)), y, y, length);
		
	}
}

/* numerically normalize the response to have 0 dB gain at the central 
	 * frequency.
	 * Right now, I am just dividing the coefficients in the numerator 
	 * of each second order stage by the gain. This can almost certainly 
	 * be improved.*/

	/* so we evaluate the transfer function of the biquad filter designated 
	 * by the 6 entries in coef.
	 * The gain is the magnitude of the transfer function evaluated when 
	 * z = exp(I* 2*PI*centralFreq/samplerate)
	 */
void numericalNormalize(float centralFreq, int samplerate, double *coef)
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

void sosCoef(float centralFreq, int samplerate, double *coef)
{
	/* taken from Slaney 1993 
	 * https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf
	 * Expects coef to be already allocated and have room for 24 
	 * entries
	 */
	double delta_t = 1./(double)samplerate;
	double cf = (double)centralFreq;
	double b = 2*M_PI*1.019*24.7*(4.37*cf/1000. + 1); //bandwidth
	int i;

	/* Now to actually set the coefficients for each stae of filtering.
	 * The only coefficient that changes between stages is b1
	 */
	for (i=0;i<4;i++){
		/* We  start by setting b0 */
		coef[6*i] = delta_t;
		/* set b1 */
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
		/* set b2 */
		coef[6*i+2] = 0;
		/* set a0 */
		coef[6*i+3] = 1;
		/* set a1 */
		coef[6*i+4] = -2*cos(2*cf*M_PI*delta_t)/exp(b*delta_t);
		/* set a2 */
		coef[6*i+5] = exp(-2*b*delta_t);

		/* now to numerically normalize */
		numericalNormalize(centralFreq, samplerate, coef+6*i);
	}
}


/* this is where the main work for the sosGammatoneFilter is done. 
 * It is maintained separately for testing. It does not upsample, downsample, 
 * or convert two and from floats 
 */
void sosGammatoneHelper(const double* data, double* output, float centralFreq,
			int samplerate, int datalen)
{
	double coef[24];
	sosCoef(centralFreq, samplerate, coef);
	cascadeBiquad(4, coef, data, output, datalen);
}


void sosGammatone(const float* data, float* output, float centralFreq,
		  int samplerate, int datalen)
{
	double *ddoubled, *dresult;
	float *fdoubled, *fresult;
	int doubled_length,result_length,i;
	// first we do data doubling - to avoid aliasing
	fdoubled = NULL;
	doubled_length = ResampleAndAlloc(data, datalen, 2, &fdoubled);
	if (doubled_length == (2*datalen-1)){
		fdoubled[doubled_length] = 0;
		doubled_length++;
	}
	assert((doubled_length == (2*datalen)));

	// next we allocate memory
	ddoubled = malloc(sizeof(double)*doubled_length);

	// convert from array of floats to array of doubles
	for (i = 0; i < doubled_length; i++){
		ddoubled[i] = (double)fdoubled[i];
	}
	// no longer need fdoubled
	free(fdoubled);

	// allocate memory for dresult
	dresult = malloc(sizeof(double)*doubled_length);

	// perform filtering
	sosGammatoneHelper(ddoubled, dresult, centralFreq,
			   2*samplerate, doubled_length);
	// no longer need ddoubled
	free(ddoubled);

	// allocate memory for fresult
	fresult = malloc(sizeof(float)*doubled_length);

	// convert the array of results from doubles to floats
	for (i = 0; i < doubled_length; i++){
		fresult[i] = (float)dresult[i];
	}

	// no longer need dresult
	free(dresult);

	/* finally we downsample back down to the starting frequency */
	result_length = ResampleAndAlloc(fresult, doubled_length, 0.5,
					 &output);
	if (result_length == (datalen-1)){
		output[result_length]=0;
		result_length++;
	}
	assert((result_length == datalen));
	free(fresult);
}







//float version of biquadFilter
void biquadFilterf(float *coef, const float *x, float *y, int length)
{
	float d1, d2, cur_x, cur_y, a0,a1,a2,b0,b1,b2;
	int n;
	/* d1 and d2 are state variables, for now asssume they start at 0,
	 * because before a recording there is silence. If we are chunking the 
	 * recording we will need to track d1 and d2 between chunks. */
	d1 = 0;
	d2 = 0;

	/* set the feedforward coefficients */
	b0 = coef[0]; b1 = coef[1]; b2 = coef[2];
	/* set the feedback coefficients */
	a0 = coef[3]; a1 = coef[4]; a2 = coef[5];

	for (n = 0; n<length; n++){
		cur_x = x[n];
		/* first difference equation: y[n] = (b0 * x[n] + d1[n-1])/a0 
		 */
		cur_y = (b0 * cur_x + d1)/a0;

		/* next difference equation: 
		 *     d1[n] = b1 * x[n] - a1 * y[n] + d2[n-1]
		 */
		d1 = b1 * cur_x - a1 * cur_y + d2;
		/* final difference equation: d2[n] = b2 * x[n] - a2 * y[n]
		 */
		d2 = b2 * cur_x - a2 * cur_y;

		/* finally set y to cur_y */
		y[n] = cur_y;
	}
}

//float version of cascadeBiquad
void cascadeBiquadf(int num_stages, float *coef, const float *x, float *y,
		    int length)
{
	/* run the filter the first time to produce y */
	biquadFilterf(coef, x, y, length);

	/* run the biquad filter num_stages-1 more times, updating y in place 
	 */
	for (int i= 1; i<num_stages; i++){
		biquadFilterf((coef + (6*i)), y, y, length);
	}
}

//float version of numericalNormalize
void numericalNormalizef(float centralFreq, int samplerate, float *coef)
{
	float TWO_PI = (float)(2 * M_PI);
	float x1, x2, gain;
	x1 = TWO_PI * centralFreq/samplerate;
	x2 = 2 * TWO_PI * centralFreq/samplerate;

	gain = sqrtf((powf(coef[2]+coef[1]*cosf(x1) + coef[0]* cosf(x2),2.0f)
                 + powf(coef[1]*sinf(x1) + coef[0] * sinf(x2),2.0f))
                /( powf(coef[5]+coef[4]*cosf(x1) + coef[3]* cosf(x2),2.0f)
                   + powf(coef[4]*sinf(x1) + coef[3] * sinf(x2),2.)));
	coef[0] = coef[0]/gain;
	coef[1] = coef[1]/gain;
	coef[2] = coef[2]/gain;
}

//float version of sosCoeff
void sosCoeff(float centralFreq, int samplerate, float* coef)
{
	/* taken from Slaney 1993 
	 * https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf
	 * Expects coef to be already allocated and have room for 24 
	 * entries
	 */
	float TWO_PI = (float)(2 * M_PI);
	float delta_t = 1.0f/samplerate;
	float b = TWO_PI*1.019f*24.7f*(4.37f*centralFreq/1000.0f + 1); //bandwidth
	int i;

	/* Now to actually set the coefficients for each stae of filtering.
	 * The only coefficient that changes between stages is b1
	 */
	for (i=0;i<4;i++){
		/* We  start by setting b0 */
		coef[6*i] = delta_t;
		/* set b1 */
		if (i<2){
			coef[6*i+1] = (-((2 * delta_t * cosf(TWO_PI * centralFreq * delta_t)
					  / expf(b * delta_t))
					 + (powf(-1,(float)i) * 2
					    * sqrtf(3 + powf(2.0f, 1.5f)) * delta_t *
					    sinf(TWO_PI * centralFreq * delta_t)
					    / expf(b * delta_t))) / 2.0f);
		} else {
			coef[6*i+1] = -(2 * delta_t * cosf(TWO_PI * centralFreq * delta_t)
					/ expf(b * delta_t)
					+ powf(-1,(float)i) * 2
					* sqrtf(3 - powf(2.0f,1.5f)) * delta_t *
					sinf(TWO_PI * centralFreq * delta_t)
					/ expf(b * delta_t)) / 2.0f;
		}		
		/* set b2 */
		coef[6*i+2] = 0;
		/* set a0 */
		coef[6*i+3] = 1;
		/* set a1 */
		coef[6*i+4] = -2*cosf(TWO_PI*centralFreq*delta_t)/expf(b*delta_t);
		/* set a2 */
		coef[6*i+5] = expf(-2*b*delta_t);

		/* now to numerically normalize */
		numericalNormalizef(centralFreq, samplerate, coef+6*i);
	}
}

void sosGammatoneFast(const float* data, float* output, float centralFreq,
		      int samplerate, int datalen)
{
	//modified sosGammatone that does not double the samplerate and uses floats instead of doubles.
	//If the loss in accuracy is negligible, we should switch to this as it is significantly faster
	float coef[24];
	sosCoeff(centralFreq, samplerate, coef);
	cascadeBiquadf(4, coef, data, output, datalen);
}
