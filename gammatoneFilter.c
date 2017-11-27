#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "samplerate.h"

double ERB(double f)
{
  //ERB = 24.7(4.37*10^-3 * f + 1)
  return (0.107939 * f) + 24.7;
}

//g(t) = a t^(n-1) e^(-2pi b t) cos(2pi f t + phase)
//n = 4
//b = 1.019 * ERB
//ERB(only when n=4) = 24.7(0.00437 f + 1)
//phase, in our case, can be safely ignored and removed.
//so with the above...
//g(t) = a t^(3) e^(-2pi t (0.109989841 f + 27.513)) cos(2pi f t)
void gammatoneNormalized(float** output, float centralFreq, int samplerate, int datalen)
{
  //returns a gammatone filter with amplitude set to 1.
  double bandwidth = 1.019 * ERB(centralFreq);
  //printf("making gammatone, datalen %d, centralFreq %f, bandwidth %f\n", datalen, centralFreq, bandwidth);
  float sum = 0.0f;
  for(int i = 0; i < datalen; i++){
    double t = i/(double)samplerate;
    (*output)[i] = (float) (pow(i,3) * exp(-2*M_PI*bandwidth*t) * cos(2*M_PI*centralFreq*t) );
    sum += (*output)[i];
  }
  //printf("sum %f\n", sum);
  for(int i = 0; i < datalen; ++i){
    (*output)[i] = (*output)[i] / sum;
  }
}

void simpleGammatone(float* data, float** output, float centralFreq, int samplerate, int datalen)
{
  float* gammatone = malloc(sizeof(float)*datalen);
  gammatoneNormalized(&gammatone, centralFreq, samplerate, datalen);

  for(int i = 0; i < datalen; ++i){
    (*output)[i] = fabs(data[i]) * gammatone[i];
  }

  free(gammatone);
}

void simpleGammatoneImpulseResponse(float* data, float** output, float centralFreq, int samplerate, int datalen)
{
  //my understanding of impulse response is gathered mostly from:
  //https://dsp.stackexchange.com/questions/536/what-is-meant-by-a-systems-impulse-response-and-frequency-response
  //https://en.wikipedia.org/wiki/Linear_time-invariant_theory#Impulse_response_and_convolution
  //but im still not entirely confident i am doing it correctly...

  //how quickly the gammatone decays is inversely proportional to its bandwidth.
  //channel 0 has the smallest bandwidth at 35.9, which decays significantly by index 1850.
  //we could probably be much stricter (even 1000 would encompass most of the 'significant' signal)
  int gammaLength = 1850;

  float* gammatone = malloc(sizeof(float)*gammaLength);
  gammatoneNormalized(&gammatone, centralFreq, samplerate, gammaLength);

  //float sum = 0.0f;
  for(int i = 0; i < datalen; ++i){
    (*output)[i] = 0.0f;
    for(int j = 0; j < gammaLength && i+j < datalen; ++j){
      (*output)[i] += fabs(data[i+j]) * gammatone[j];
      //(*output)[i] += data[i+j] * gammatone[j];
    }
    //sum += (*output)[i];
  }
  //printf("sum %f\n", sum);
  free(gammatone);
}

/* MA note: I don't know a lot about Impulse Response Filtering, but going off 
 * my previous experience using filtering in image processing, I assume that 
 * to apply a gammatone impules filter one convolves the impulse response with 
 * the input signal. The above function does not do that. Applying that seems 
 * really, costly so I might not know what I am talking about. Below I attempt 
 * to implement a version taken from a paper*/



int dataDoubling(float* data,int datalen, int samplerate, float **doubled){
	/* Helper function for naiveGammatone. It handles data doubling. It 
	 * might be fine just to repeat entries, but I am not sure how we 
	 * would handle downsampling - thus we will use libsamplerate  */

	int success_code,result_length;

	(*doubled) = malloc(sizeof(float)* 2*datalen); 
	SRC_DATA *resampleData = malloc(sizeof(SRC_DATA));
	resampleData -> data_in = data;
	resampleData -> data_out = *doubled;
	resampleData -> input_frames = (long)datalen;
	resampleData -> output_frames = (long)(2 * datalen);
	resampleData -> src_ratio = 2.0;

	success_code = src_simple(resampleData, 0, 1);

	if (success_code != 0){
		printf("libsamplerate Error:\n %s\n",
		       src_strerror(success_code));
		free(resampleData);
		free(doubled);
		return -1;
	}

	result_length = (int)(resampleData->output_frames_gen);
	free(resampleData);
	return result_length;
}

int dataHalving(float* data,int datalen, int samplerate, float **halved){
	/* Helper function for naiveGammatone. It handles data halving*/

	int success_code,result_length;

	(*halved) = malloc(sizeof(float)* datalen/2); 
	SRC_DATA *resampleData = malloc(sizeof(SRC_DATA));
	resampleData -> data_in = data;
	resampleData -> data_out = *halved;
	resampleData -> input_frames = (long)datalen;
	resampleData -> output_frames = (long)(datalen/2);
	resampleData -> src_ratio = 0.5;

	success_code = src_simple(resampleData, 0, 1);

	if (success_code != 0){
		printf("libsamplerate Error:\n %s\n",
		       src_strerror(success_code));
		free(resampleData);
		free(halved);
		return -1;
	}

	result_length = (int)(resampleData->output_frames_gen);
	free(resampleData);
	return result_length;
}

void first_order_recursive_filter(float* re_z, float* im_z, float* re_w,
				   float* im_w, int length, float b,
				   float delta_t){
	/* Helper function that computes the response from the first-order.
	 * This takes an arbitrary complex input array z[k] and returns the 
	 * complex output array w[k]. Our representation of complex arrays is 
	 * consistent with our treatment in naiveGammatone:
	 *     z[k] = re_z[k] + i * im_z[k]
	 *     w[k] = re_w[k] + i * im_w[k]
	 * the filter computes element k using the following formula:
	 * w[k] = w[k-1] + (1-exp(-2*pi*b*delta_t)) * (z[k-1] - w[k-1])
	 */

	int k;
	float factor;

	/* for convenience, factor = (1-exp(-2*pi*b*delta_t)) */
	factor = 1. - expf(-2.*M_PI*b*delta_t);

	/* We require special handling for w[0] because it requires data from 
	 * before the start of the input. We simply assume silence for that 
	 * data - thus w[k<0] = 0 and z[k<0] = 0 
	 */
	re_w[0] = 0;
	im_w[0] = 0;
	for (k=1;k<length;k++){
		re_w[k] = re_w[k-1] + factor * (re_z[k-1]-re_w[k-1]);
		im_w[k] = im_w[k-1] + factor * (im_z[k-1]-im_w[k-1]);
	}
}

void swap_arrays(float** array1, float** array2){
	float *temp = *array1;
	*array1 = *array2;
	*array2 = temp;
}

void recursive_filter_application(float* re_z, float* im_z, float* re_w,
				  float* im_w, int length, float b,
				  float delta_t){
	/* Helper function that recursively applies the first-order recursive 
	 * filter function on its output to approximate the 4th order gammatone 
	 * filter. 
	 */
	int i;
	for (i=0; i<4; i++){
		if (i>0){
			/* we call the first_order_recursive_filter, z[k] 
			 * represents our input and w[k] represents our output.
			 * Here, we are setting z[k] to the output of the prior 
			 * first_order_recursive_filter call and we are setting 
			 * w[k] to the input of the prior 
			 * first_order_recursive_filter call (so that it can be 
			 * overwritten 
			 */
			swap_arrays(&re_z, &re_w);
			swap_arrays(&im_z, &im_w);
		}
		first_order_recursive_filter(re_z, im_z, re_w, im_w, length, b,
					     delta_t);
	}
}

/* Drawing slighlty from simpleImplementation of GammatoneFilter 
 * Draws mostly from: 
 * https://www.pdn.cam.ac.uk/other-pages/cnbh/files/publications/
 * SVOSAnnexC1988.pdf
 *
 * Basically we use a cascade of recursive filters - I have not done anything 
 * to try to offset them. I also have made no assumptions about the 
 * normalization constant - I am not sure what this approximation does for our 
 * normalization constant.
 *
 * Need to take a look at our resampling - it always returns arrays that are 1 
 * elemet shorter than we want - right now we just set the final entry to 0, 
 * but we need to be more clever.
 * 
 * This was written to be as transparent as possible. With minimal modification 
 * you could make it more memory.
 * There is almost definitely to apply these operations element-wise instead of 
 * performing array operations (especially since we are recursively applying 
 * only 4 filters.
 */
void naiveGammatone(float* data, float** output, float centralFreq,
		    int samplerate, int datalen)
{
	int k, doubled_length, result_length;
	float *doubled, *re_z, *im_z, *re_w, *im_w, *y, delta_t,phi,b;

	// first we do data doubling - to avoid aliasing
	doubled = NULL;
	doubled_length = dataDoubling(data,datalen, samplerate, &doubled);
	if (doubled_length == (2*datalen-1)){
		doubled[doubled_length] = 0;
		doubled_length++;
	}
	//printf("doubled_length = %d\n", doubled_length);
	//printf("twice the input length = %d\n", 2*datalen);
	assert((doubled_length == (2*datalen)));

	re_z = malloc(sizeof(float)*doubled_length);
	im_z = malloc(sizeof(float)*doubled_length);
	re_w = malloc(sizeof(float)*doubled_length);
	im_w = malloc(sizeof(float)*doubled_length);

	/* we need to frequency shift the array x[k] by the amout -f0. Here
	 * x[k] as doubled. z[k] is the complex array which we represent as:
	 *     z[k] = re_z[k] + i * im_z[k]
	 * z[k] = exp(-i*2*pi*f_0 * k * delta_t) * x[k]
	 * here delta_t is 1/(2*samplerate) - the factor of 2 accounts for how 
	 * we doubled the samplerate. To solve for the elements of re_z and 
	 * im_z, we use Euler's equation: exp(i*phi) = cos(phi) + i * sin(phi)
	 */

	delta_t = 0.5/samplerate;
	b = 1.019 * ERB(centralFreq);

	for (k=0; k<doubled_length; k++){
		/* z[k] = x[k] * exp(-i*2*pi*f_0 * k * delta_t)
		 * z[k] = x[k] * (cos(2*pi*f_0 * k * delta_t) 
		 *                 - i * sin(2*pi*f_0 * k * delta_t))
		 * Re(z[k]) = x[k] * cos(2*pi*f_0 * k * delta_t)
		 * Im(z[k]) = -1* x[k] * sin(2*pi*f_0 * k * delta_t)
		 */
		phi = 2*M_PI*centralFreq * delta_t*k;
		re_z[k] = doubled[k] * cos(phi);
		im_z[k] = -1 * doubled[k] * sin(phi);
	}

	/* now we can free doubled */
	free(doubled);

	/* recursively apply the first-order recursive filter */
	recursive_filter_application(re_z, im_z, re_w, im_w, doubled_length,
				     b, delta_t);
	/* we can free z*/
	free(re_z);
	free(im_z);

	y = malloc(sizeof(float)*doubled_length);
	/* Here we need to shift the frequency of the output of the filter by 
	 * +f0 and take the real component to produce an array y[k] 
	 * y[k] = Re( exp(i*2*pi*f_0 * k * delta_t) * w[k])
	 * y[k] = Re((re_w[k] + i*im_w[k]) * (cos(2*pi*f_0*k *delta_t) 
	 *                                     + i*sin(2*pi*f_0*k*delta_t)))
	 * y[k] = re_w[k] * cos(2*pi*f_0*k *delta_t) 
	 *          - im_w[k] * sin(2*pi*f_0*k*delta_t)
	 */
	
	for (k=0; k<doubled_length; k++){
    phi = 2*M_PI*centralFreq * delta_t*k;
    y[k] = (re_w[k]*cos(phi) - im_w[k] * sin(phi));
  }
	
	/* finally we downsample back down to the starting frequency */
	result_length = dataHalving(y, doubled_length, 2*samplerate, output);
	
	//printf("result_length = %d\n", result_length);
	//printf("input length = %d\n", datalen);
	if (result_length == (datalen-1)){
		(*output)[result_length]=0;
		result_length++;
	}
	assert((result_length == datalen));	
	free(re_w);
	free(im_w);
	free(y);
}

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
void biquadFilter(double *coef, double *x, double *y, int length){
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
		   int length){

	/* run the filter the first time to produce y */
	biquadFilter(coef, x, y, length);

	/* run the biquad filter num_stages-1 more times, updating y in place 
	 */
	for (int i= 1; i<num_stages; i++){
		biquadFilter((coef + (6*i)), y, y, length);
		
	}
}

void numericalNormalize(float centralFreq, int samplerate, double *coef){
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

void allPoleCoef(float centralFreq, int samplerate, double *coef){
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
		coef[6*i+1] = (-((2 * delta_t * cos(2 * cf * M_PI * delta_t)
				  / exp(b * delta_t))
				 + (pow(-1,(double)i) * 2
				    * sqrt(3 + pow(2., 1.5)) * delta_t *
				    sin(2 * cf * M_PI * delta_t)
				    / exp(b * delta_t))) / 2.);
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


/* this is where the main work for the allPoleGammatoneFilter is done. 
 * It is maintained separately for testing. It does not upsample, downsample, 
 * or convert two and from floats 
 */
void allPoleGammatoneHelper(double* data, double** output, float centralFreq,
			    int samplerate, int datalen){
	double *coef = malloc(sizeof(double)*24);
	allPoleCoef(centralFreq, samplerate, coef);
	cascadeBiquad(4, coef, data, (*output), datalen);
	free(coef);
}


/* The allPoleGammatone approximates the gammatone function. 
 * The All-Pole Gammatone Filter (APGF) is described by Slaney 1993:
 *    https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf
 * Our entire implementation is based on his description (It is implemented 
 * using a cascade of 4 biquad filters).
 *
 * I believe there is further disccusionabout the filter by Lyon 1996:
 *   http://www.dicklyon.com/tech/Hearing/APGF_Lyon_1996.pdf
 * I believe that they may also discuss how to normalize the frequency 
 * response. He also discuses how the One-Zero Gammatone Filter (OZGF) is a 
 * better approximation for a gammatone than the APGF. It is slightly modified 
 * from the APGF.
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
 */
void allPoleGammatone(float* data, float** output, float centralFreq,
		      int samplerate, int datalen){
	double *ddoubled, *dresult;
	float *fdoubled, *fresult;
	int doubled_length,result_length,i;
	// first we do data doubling - to avoid aliasing
	fdoubled = NULL;
	doubled_length = dataDoubling(data,datalen, samplerate, &fdoubled);
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
	allPoleGammatoneHelper(ddoubled, &dresult, centralFreq,
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
	result_length = dataHalving(fresult, doubled_length, 2*samplerate,
				    output);
	if (result_length == (datalen-1)){
		(*output)[result_length]=0;
		result_length++;
	}
	assert((result_length == datalen));
	free(fresult);
}






int compareArrayEntries(double *ref, double* other, int length,
			double tol, int rel,double abs_zero_tol){
	// if relative < 1, then we compare absolute value of the absolute
	// difference between values
	// if relative > 1, we need to specially handle when the reference
	// value is zero. In that case, we look at the absolute differce and
	// compare it to abs_zero_tol
	
	int i;
	int success = 1;
	double diff;
	
	for (i = 0; i< length; i++){
		diff = abs(ref[i]-other[i]);
		if (rel >=1){
			if (ref[i] == 0){
				// we will just compute relative difference
				if (diff > abs_zero_tol){
					success = -1;
					printf("ref[%d] = 0, comp[%d] = %e",
					       i,i,other[i]);
					printf(" has abs diff > %e\n",
					       abs_zero_tol);
				}
				continue;
			}
			diff = diff/abs(ref[i]);
		}
		if (diff>tol){
			success = -1;
		      	printf("ref[%d] = %e, comp[%d] = %e", i,ref[i],
			       i,other[i]);
			if (rel>=1){
				printf(" has rel diff > %e\n", tol);
			} else {
				printf(" has abs diff > %e\n", tol);
			}
		}		
	}
	return success;

}

int testAllPoleCoefFramework(float centralFreq, int samplerate, double *ref,
			     double tol, int rel, double abs_zero_tol){
	double *coef = malloc(sizeof(double)*24);
	allPoleCoef(centralFreq, samplerate, coef);
	int r = compareArrayEntries(ref, coef, 24, tol, rel, abs_zero_tol);
	free(coef);
	return r;
}
		

int testAllPoleCoef(){
	double ref[] = {6.1031107e-02, -1.2118071e-01, 0., 1., -1.5590685e+00,
			8.5722460e-01, 5.5986645e-02, 2.3877628e-02, 0., 1.,
			-1.5590685e+00, 8.5722460e-01, 1.3816354e-01,
			-1.3629211e-01, 0., 1., -1.5590685e+00, 8.5722460e-01,
			1.2797372e-01, -7.3279486e-02, 0., 1., -1.5590685e+00,
			8.5722460e-01};
	
	double ref1[] = {9.1368911e-02, -2.0813450e-01, 0., 1., -9.8759824e-01,
			 7.8999199e-01, 8.6315676e-02, 1.1137823e-01, 0., 1.,
			 -9.8759824e-01, 7.8999199e-01, 2.0164386e-01,
			 -1.6129740e-01, 0., 1., -9.8759824e-01, 7.8999199e-01,
			 1.9219808e-01, -3.6072876e-02, 0., 1., -9.8759824e-01,
			 7.8999199e-01};
	double ref2[] = {2.5925251e-02, -3.0544150e-02, 0., 1., -1.9335553e+00,
			 9.4232544e-01, 2.0521109e-02, -1.5501504e-02, 0., 1.,
			 -1.9335553e+00, 9.4232544e-01, 5.8263388e-02,
			 -5.8440828e-02, 0., 1., -1.9335553e+00, 9.4232544e-01,
			 4.7312338e-02, -4.4024593e-02, 0., 1., -1.9335553e+00,
			 9.4232544e-01};
	int r, success;
	int rel = 1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;
	
	success = 1;
	printf("AllPole Coefficient Test 1:\n");
	r = testAllPoleCoefFramework(1000., 11025, ref,
				     tol, rel, abs_zero_tol);
	if (r == 1){
		printf("Passed\n");
	} else {
		success = -1;
		printf("Failed\n");
	}

	printf("AllPole Coefficient Test 2:\n");
	r = testAllPoleCoefFramework(2500., 16000, ref1,
				     tol, rel, abs_zero_tol);
	if (r == 1){
		printf("Passed\n");
	} else {
		success = -1;
		printf("Failed\n");
	}

	printf("AllPole Coefficient Test 3:\n");
	r = testAllPoleCoefFramework(115., 8000, ref2,
				     tol, rel, abs_zero_tol);
	if (r == 1){
		printf("Passed\n");
	} else {
		success = -1;
		printf("Failed\n");
	}
	return success;
}


void stepFunction(double *array,int length, double start, double increment,
		 int steplength){
	double cur;
	int i=0;
	int j=1;
	cur = start;
	while (i<length){
		if (i < j*steplength){
			array[i] = cur;
		} else {
			j++;
			cur = increment*(double)j+start;
			array[i] = cur;
		}
		i++;
	}
}

int testAllPoleGammatoneFramework(float centralFreq, int samplerate,
				  double *input, int length, double *ref,
				  double tol, int rel, double abs_zero_tol){

	double *result = malloc(sizeof(double)*length);
	allPoleGammatoneHelper(input, &result, centralFreq, samplerate, length);
	int r = compareArrayEntries(ref, result, length, tol, rel,
				    abs_zero_tol);
	free(result);
	return r;
}

int testAllPoleGammatone(){
	
	double *impulse_input = calloc(1000,sizeof(double));
	impulse_input[0] = 1.;

	double *input2 = calloc(100,sizeof(double));
	stepFunction(input2,100, -2.0, 3.0,17);

	double ref1[] = {6.7683936e-17, 2.1104779e-16, 2.4239155e-16,
			 -1.4876671e-16, -1.1331346e-15, -2.4695411e-15,
			 -3.4440334e-15, -3.1343673e-15, -9.2002259e-16,
			 3.0133997e-15, 7.4745466e-15, 1.0557555e-14,
			 1.0381404e-14, 5.9970156e-15, -1.9466179e-15,
			 -1.1132757e-14, -1.8233237e-14, -2.0110857e-14,
			 -1.5126992e-14, -4.0193326e-15, 1.0060513e-14,
			 2.2454080e-14, 2.8581038e-14, 2.5646008e-14,
			 1.3823791e-14, -3.5618239e-15, -2.0997375e-14,
			 -3.2589734e-14, -3.4111317e-14, -2.4553863e-14,
			 -6.6166725e-15, 1.4079451e-14, 3.0812802e-14,
			 3.8005663e-14, 3.3124484e-14, 1.7576952e-14,
			 -3.7107607e-15, -2.3900910e-14, -3.6494625e-14,
			 -3.7467944e-14, -2.6578874e-14, -7.4166384e-15,
			 1.3808726e-14, 3.0317848e-14, 3.6963977e-14,
			 3.1859387e-14, 1.6907246e-14, -2.9066224e-15,
			 -2.1196912e-14, -3.2261313e-14, -3.2875479e-14,
			 -2.3231246e-14, -6.7477481e-15, 1.1139880e-14,
			 2.4784009e-14, 3.0117890e-14, 2.5866809e-14,
			 1.3818143e-14, -1.8902313e-15, -1.6188239e-14,
			 -2.4712622e-14, -2.5136122e-14, -1.7783601e-14,
			 -5.3805073e-15, 7.9372922e-15, 1.7997285e-14,
			 2.1892786e-14, 1.8809500e-14, 1.0146682e-14,
			 -1.0601169e-15, -1.1188766e-14, -1.7193376e-14,
			 -1.7509479e-14, -1.2436708e-14, -3.9158586e-15,
			 5.1876918e-15, 1.2036074e-14, 1.4693514e-14,
			 1.2657383e-14, 6.9063074e-15, -5.1340397e-16,
			 -7.1998351e-15, -1.1162608e-14, -1.1403387e-14,
			 -8.1447921e-15, -2.6647521e-15, 3.1810314e-15,
			 7.5759533e-15, 9.2970380e-15, 8.0412438e-15,
			 4.4418458e-15, -2.0430209e-16, -4.3903504e-15,
			 -6.8789815e-15, -7.0581776e-15, -5.0744952e-15,
			 -1.7221623e-15, 1.8566496e-15, 4.5517834e-15,
			 5.6215961e-15};
	double ref2[] = {2.4414063e-16, 1.1882594e-15, 3.4514247e-15,
		       7.7549994e-15, 1.4852864e-14, 2.5456417e-14,
		       4.0157853e-14, 5.9355054e-14, 8.3181170e-14,
		       1.1144163e-13, 1.4356102e-13, 1.7854179e-13,
		       2.1493645e-13, 2.5083429e-13, 2.8386344e-13,
		       3.1120843e-13, 3.2964319e-13, 3.3631121e-13,
		       3.2868989e-13, 3.0451932e-13, 2.6170466e-13,
		       1.9820115e-13, 1.1189041e-13, 4.5659077e-16,
		       -1.3873032e-13, -3.0871686e-13, -5.1304851e-13,
		       -7.5582707e-13, -1.0417395e-12, -1.3760553e-12,
		       -1.7645912e-12, -2.2136427e-12, -2.7298833e-12,
		       -3.3202329e-12, -3.9909660e-12, -4.7476247e-12,
		       -5.5949606e-12, -6.5368976e-12, -7.5765119e-12,
		       -8.7160225e-12, -9.9567894e-12, -1.1299313e-11,
		       -1.2743233e-11, -1.4287324e-11, -1.5929484e-11,
		       -1.7666719e-11, -1.9495116e-11, -2.1409816e-11,
		       -2.3404968e-11, -2.5473697e-11, -2.7608049e-11,
		       -2.9798222e-11, -3.2032611e-11, -3.4297925e-11,
		       -3.6579333e-11, -3.8860657e-11, -4.1124595e-11,
		       -4.3352960e-11, -4.5526942e-11, -4.7627368e-11,
		       -4.9634971e-11, -5.1530652e-11, -5.3295727e-11,
		       -5.4912167e-11, -5.6362813e-11, -5.7631573e-11,
		       -5.8703596e-11, -5.9565416e-11, -6.0204341e-11,
		       -6.0608649e-11, -6.0767797e-11, -6.0672650e-11,
		       -6.0315712e-11, -5.9691347e-11, -5.8795990e-11,
		       -5.7628334e-11, -5.6189486e-11, -5.4483092e-11,
		       -5.2515422e-11, -5.0295408e-11, -4.7834644e-11,
		       -4.5147333e-11, -4.2250192e-11, -3.9162308e-11,
		       -3.5904956e-11, -3.2500639e-11, -2.8972922e-11,
		       -2.5346287e-11, -2.1645992e-11, -1.7897941e-11,
		       -1.4128551e-11, -1.0364613e-11, -6.6331555e-12,
		       -2.9612847e-12, 6.2397815e-13, 4.0958775e-12,
		       7.4281174e-12, 1.0595070e-11, 1.3571994e-11,
		       1.6335269e-11};
	
	int r;
	int rel = 1;
	int success =1;
	double tol = 1.e-5;
	double abs_zero_tol = 1.e-5;

	printf("All-Pole Gammatone Response Test 1:\n");
	r = testAllPoleGammatoneFramework(1000, 11025, impulse_input, 100,
					  ref1, tol, rel, abs_zero_tol);
	if (r == 1){
		printf("Passed\n");
	} else {
		success = -1;
		printf("Failed\n");
	}

	printf("All-Pole Gammatone Response Test 2:\n");
	r = testAllPoleGammatoneFramework(115, 8000, input2, 100,
					  ref2, tol, rel, abs_zero_tol);
	if (r == 1){
		printf("Passed\n");
	} else {
		success = -1;
		printf("Failed\n");
	}

	free(impulse_input);
	free(input2);
	return success;

}

/*
int main(int argc, char ** argv)
{
	testAllPoleCoef();
	// need to update the expected results in testAllPoleGammatone
	//testAllPoleGammatone();
	return 0;
}
*/
