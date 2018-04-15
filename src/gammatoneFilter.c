#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "resample.h"


/* MA note: I don't know a lot about Impulse Response Filtering, but going off 
 * my previous experience using filtering in image processing, I assume that 
 * to apply a gammatone impules filter one convolves the impulse response with 
 * the input signal. The above function does not do that. Applying that seems 
 * really, costly so I might not know what I am talking about. Below I attempt 
 * to implement a version taken from a paper*/



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
void biquadFilter(double *coef, double *x, double *y, int length)
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
void cascadeBiquad(int num_stages, double *coef, double *x, double *y,
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
void sosGammatoneHelper(double* data, double** output, float centralFreq,
			    int samplerate, int datalen)
{
	double *coef = malloc(sizeof(double)*24);
	sosCoef(centralFreq, samplerate, coef);
	cascadeBiquad(4, coef, data, (*output), datalen);
	free(coef);
}


/* The sosGammatone approximates the gammatone function. 
 *
 * The sos Gammatone Filter is described by Slaney 1993:
 *    https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf
 * Our entire implementation is based on his description (It is implemented 
 * using a cascade of 4 biquad filters or using 4 second order sections - sos).
 *
 * Need to be careful about sampling rate. We can definitely run into aliasing 
 * problems due to the Nyquist Frequency being close to the highest central 
 * frequency depending on the input. For now we will just upsample and 
 * downsample like with the naiveGammatone function (not exactly right, but 
 * close).
 *
 * To start with, this is implemented using doubles - we can probably make this 
 * work on floats but we need to be careful as precision does matter.
 *
 * This function can certainly be optimized. Currently the cascade filter calls 
 * the biquadFilter 4 times meaning that we iterate over the array 4 times. We 
 * could consolidate that into filtering one time through. We could optimize 
 * the biquadFilter function to expect a0 = 1 (which is standard and would 
 * remove datalen*4 unnecessary multiplications). If we know that we are 
 * definitely using this function, we could also modify our filtering to know 
 * that b2 is always 0 (this would remove datalen*4 unnecessary multiplications
 * and subtractions).
 *
 * Note that in the future, it may be worthwhile to implement a different 
 * approximation of the gammatone function.
 * The All-Pole Gammatone Filter (APGF) is described by Slaney 1993:
 *   https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf
 * The all-pole Gammatone filter should have better complexity.
 * I believe there is further disccusionabout the filter by Lyon 1996:
 *   http://www.dicklyon.com/tech/Hearing/APGF_Lyon_1996.pdf
 * He also discuses how the One-Zero Gammatone Filter (OZGF) is a 
 * better approximation for a gammatone than the APGF. It is slightly modified 
 * from the APGF.
 */
void sosGammatone(float* data, float** output, float centralFreq,
		      int samplerate, int datalen)
{
	double *ddoubled, *dresult;
	float *fdoubled, *fresult;
	int doubled_length,result_length,i;
	// first we do data doubling - to avoid aliasing
	fdoubled = NULL;
	doubled_length = Resample(&data, datalen, 2, &fdoubled);
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
	sosGammatoneHelper(ddoubled, &dresult, centralFreq,
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
	result_length = Resample(&fresult, doubled_length, 0.5,
				    output);
	if (result_length == (datalen-1)){
		(*output)[result_length]=0;
		result_length++;
	}
	assert((result_length == datalen));
	free(fresult);
}








// version of biquadFilter that starts with an array of floats and returns an
// array of floats
void biquadFilterdf(double *coef, float *x, double *y, int length)
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

//version of cascadeBiquad that accepts floats and returns doubles
void cascadeBiquaddf(int num_stages, double *coef, float *x, double *y,
		     int length)
{
	/* run the filter the first time to produce y */
	biquadFilterdf(coef, x, y, length);

	/* run the biquad filter num_stages-1 more times, updating y in place 
	 */
	for (int i= 1; i<num_stages; i++){
		biquadFilter((coef + (6*i)), y, y, length);
	}
}


void sosGammatoneFast(float* data, float** output, float centralFreq,
		      int samplerate, int datalen)
{
	/* It turns out that 27 indices into a simple IR calculation with a 
	 * central Freq of ~97.618416 Hz and a samplerate of 11025 Hz, you get 
	 * relative differences in the response of slightly greater than 
	 * 5.e-4. Since these errors are only going to compound, we are going 
	 * to switch back to using doubles for the coefficients (it may 
	 * ultimately turn out that this change is inconsequential. But, for 
	 * now, while we are worrying about correctness, we are just going to 
	 * rely on the use of doubles (plus the change of floats to doubles is 
	 * negligible in comparison to other parts of onset offset function).
	 *
	 * It still remains to be seen just how much the lack of data doubling 
	 * influences the accuracy.
	 *
	 * There is an obvious way to improve performance here. Rather than 
	 * iterating over the results of the filter and then going through to 
	 * convert the array into floats, we can do all operations for a 
	 * single element all at once. For more details see filterBank.c in the 
	 * StreamedDetFunction Branch.
	 */
	//modified sosGammatone that does not double the samplerate
	double *coef = malloc(sizeof(double)*24);
	sosCoef(centralFreq, samplerate, coef);
	double *temp = malloc(sizeof(double)*datalen);
	cascadeBiquaddf(4, coef, data, temp, datalen);
	free(coef);
	for (int i=0;i<datalen;i++){
		(*output)[i] = temp[i];
	}
	free(temp);
}
