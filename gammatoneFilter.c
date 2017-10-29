#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "samplerate.h"

double ERB(double f)
{
  //ERB = 24.7(4.37*10^-3 * f + 1)
  return (0.107939 * f) + 27;
}

//g(t) = a t^(n-1) e^(-2pi b t) cos(2pi f t + phase)
//n = 4
//b = 1.019 * ERB
//ERB(only when n=4) = 24.7(0.00437 f + 1)
//phase, in our case, can be safely ignored and removed.
//so with the above...
//g(t) = a t^(3) e^(-2pi t (0.109989841 f + 27.513)) cos(2pi f t)
void simpleGammatone(float* data, float** output, float centralFreq, int samplerate, int datalen)
{
  for(int i = 0; i < datalen; i++){
    double t = i/(double)samplerate;
    double bandwidth = 1.019 * ERB(centralFreq);
    double amplitude = data[i]; //this is (most likely) not actually the amplitude!!!

    (*output)[i] = amplitude * pow(i, 3) * exp(-2 * M_PI * bandwidth * t) * cos(2 * M_PI * centralFreq * t);
  }
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
		    int samplerate, int datalen){
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


