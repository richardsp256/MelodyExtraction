#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <string.h>

void compute_cluster_diffs(float* centers, float* cluster_diffs,
			   int num_centers, int* min_dist_locs,
			   float *min_diff, int num_entries,
			   float sigma, int stop_n){
	/* computes the entries of cluster differences for all powers of n
	 * from n=0 through n=stop_n-1 and populate cluster_diffs
	 *
	 * the cluster difference for arbitrary power n, and arbitrary 
	 * center[i] will be stored at cluster_diffs[i*stop_n + n]
	 * Think about cluster_diffs as a num_centers by stop_n array.
	 *
	 * cluster_diffs[i,n] is the summation (over all elements 
	 * for which centers[i] is the closest center) of the difference, 
	 * between centers[i] and an element for which centers[i] is the 
	 * closest center, quantity raised to power of n. The resulting 
	 * sum of every term is divided by (2 * sigma)^n 
	 * To be clear every individual difference in the summation is raised 
	 * to power.*/
	int i,j,offset,n;
	float cur_diff,temp,denom;

	for (i=0;i<(num_centers*stop_n);i++){
		cluster_diffs[i] = 0;
	}

	for (j=0; j< num_entries;j++){
		i = min_dist_locs[j];
		cur_diff = min_diff[j];

		offset = i*stop_n;
		// handle when n = 0
		cluster_diffs[offset] += 1;
		temp = 1;
		// handle when n > 0
		for (n = 1; n<stop_n; n++){
			temp *=cur_diff;
			cluster_diffs[offset+n] += temp;
		}
	}
	/* Now, we just need to divide all entries by (2*sigma)^n. Note that 
	 * when n = 0 we are just dividing by 1 */
	temp = 1;
	denom = 1./2*sigma;
	// handle when n>0
	for (n=1; n<stop_n;n++){
		temp*=denom;
		for (i=0;i<num_centers;i++){
			cluster_diffs[i*stop_n + n]*=temp;
		}
	}
}

static inline float fgtComponent(int n, float x){
	//returns a component of the FGT for term n and value z
	//this component is equal to (1/n!)*(hermite(n,z)/e^(-z^2))
	//the division by e^(-z^2) is bc we dont compute the e^(-z^2) 
	//component of the hermite for each term n, instead multiplying
	//the sum of the terms by e^(-z^2) at the end.
	//For speed, the computation for each term n is hardcoded.

	switch(n){
	    case 0 :
			return 1;
		case 1 :
			return 2*x;
		case 2 :
			return 2*x*x - 1;
		case 3 : //6
			return (1.3333f*x*x - 2)*x;
		case 4 : //24
			return 0.66667f*powf(x,4) - 2*powf(x,2) + 0.5f;
		case 5 : //32*powf(x,5) - 160*powf(x,3) + 120*x    / 120
			return 0.26667f*powf(x,5) - 1.3333f*powf(x,3) + x;
		case 6 : //64*powf(x,6) - 480*powf(x,4) + 720*powf(x,2) - 120    / 720
			return 0.088889f*powf(x,6) - 0.66667f*powf(x,4) + powf(x,2)
				- 0.16667f;
		case 7 : //128*powf(x,7) - 1344*powf(x,5) + 3360*powf(x,3) - 1680*x    / 5040
			return 0.025397f*powf(x,7) - 0.26667f*powf(x,5) + 0.66667f*powf(x,3)
				- 0.33333f*x;
		case 8 : //256*powf(x,8) - 3584*powf(x,6) + 13440*powf(x,4) - 13440*powf(x,2) + 1680    / 40,320
			return 0.0063492f*powf(x,8) - 0.088889f*powf(x,6) + 0.33333f*powf(x,4)
				- 0.33333f*powf(x,2) + 0.041667f;
	}
	return 0;
}

int comp(const void * elem1, const void * elem2) 
{
    float f = *((float*)elem1);
    float s = *((float*)elem2);
    return (f > s) - (f < s);
}

void sorted_clustering(int num_centers, float* centers, float *min_diff,
			      int *min_dist_locs, float *x, int length_x, float* center_offsets, int* cluster_sizes){
	/* a simple clustering which spreads centers evenly over sorted array.
	*
	* Also computes min_diff and min_dist_locs. 
	* min_dist_locs[i] holds the index of the center closest to x[i]
	* min_diff[i] holds the distance between x[i] and its closest center
	*
	* While sorted clustering is much faster than farthest point for a large number of centers,
	* it is slightly slower for a small (< 10) number of centers.
	* However, it takes a fewer number of centers to acheive high accuracy,
	* which speeds up the overall FGT.
	*/

	float sorted[length_x]; //= copy mem from x
	memcpy(sorted, x, length_x * sizeof(float));
	int i;

	qsort (sorted, length_x, sizeof(float), comp); //sort

	//get num_centers evenly spaced indices over sorted using Bresenham's line algorithm
	//by getting centers from sorted array, our centers array is also sorted

	int offset = length_x/(2*num_centers);
	for(i = 0; i < num_centers; i++){
		centers[i] = sorted[((i*length_x)/num_centers) + offset];
	}

	//compute min_diff and min_dist_locs, and recompute centers as means.
	//sorted_clustering_recompute(num_centers, centers, min_diff, min_dist_locs, x, length_x);

	//now compute min_diff anf min_dist_locs
	int index;
	int maxIndex = num_centers - 1;

	//center_offsets[i] holds the avg distance from center[i] to its points, used to update the centers.
	for(i = 0; i < num_centers; i++){
		center_offsets[i] = 0.0f;
		cluster_sizes[i] = 0;
	}

	//now compute min_diff and min_dist_locs
	for(i = 0; i < length_x; i++)
	{
		index = 0;
		while(index < num_centers && centers[index] < x[i]){
			index++;
		}

		if(index == 0){
			min_dist_locs[i] = 0;
			min_diff[i] = centers[0] - x[i];
		}else if(index == num_centers){
			min_dist_locs[i] = maxIndex;
			min_diff[i] = centers[maxIndex] - x[i];
		}else if((centers[index] - x[i]) < (x[i] - centers[index-1])){
			min_dist_locs[i] = index;
			min_diff[i] = centers[index] - x[i];
		}else{
			min_dist_locs[i] = index-1;
			min_diff[i] = centers[index-1] - x[i];
		}

		center_offsets[min_dist_locs[i]] += min_diff[i];
		cluster_sizes[min_dist_locs[i]] += 1;
	}

	//update centers
	//note: we can be sure that centers stay sorted as they are updated.
	for(i = 0; i < num_centers; i++){
		if(cluster_sizes[i] != 0){ //check if center has no points in its cluster. unlikely, but possible if we go a large number of rows without full update
			center_offsets[i] /= cluster_sizes[i];
		}
		centers[i] -= center_offsets[i];
	}

	//adjust dists to new centers
	for(i = 0; i < length_x; ++i){
		min_diff[i] -= center_offsets[min_dist_locs[i]];
	}
}

void sorted_clustering_recompute(int num_centers, float* centers, float* min_diff,
			      int *min_dist_locs, float *x, int length_x, float* center_offsets, int* cluster_sizes)
{	
	int index, i;
	int maxIndex = num_centers - 1;

	//center_offsets[i] holds the avg distance from center[i] to its points, used to update the centers.
	for(i = 0; i < num_centers; i++){
		center_offsets[i] = 0.0f;
		cluster_sizes[i] = 0;
	}

	//now compute min_diff and min_dist_locs
	for(i = 0; i < length_x-1; i++)
	{
		index = min_dist_locs[i+1]; //bc for future rows, array is essentially shifted one index, we check i+1.
		
		if(centers[index] > x[i]){
			while(index > 0 && centers[index-1] > x[i]){
				index--;
			}
		}
		else{
			while(index < num_centers && centers[index] < x[i]){
				index++;
			}
		}

		if(index == 0){
			min_dist_locs[i] = 0;
			min_diff[i] = centers[0] - x[i];
		}else if(index == num_centers){
			min_dist_locs[i] = maxIndex;
			min_diff[i] = centers[maxIndex] - x[i];
		}else if((centers[index] - x[i]) < (x[i] - centers[index-1])){
			min_dist_locs[i] = index;
			min_diff[i] = centers[index] - x[i];
		}else{
			min_dist_locs[i] = index-1;
			min_diff[i] = centers[index-1] - x[i];
		}

		center_offsets[min_dist_locs[i]] += min_diff[i];
		cluster_sizes[min_dist_locs[i]] += 1;
	}

	//last point is new, so just search up from 0
	i = length_x-1;
	index = 0;
	while(index < num_centers && centers[index] < x[i]){
		index++;
	}

	//center for x[i] is now either centers[index], or centers[index - 1]
	//guaranteed that centers[index] >= x[i], centers[index-1] <= x[i], if centers[index] and centers[indes-1] are in range
	if(index == 0){
		min_dist_locs[i] = 0;
		min_diff[i] = centers[0] - x[i];
	}else if(index == num_centers){
		min_dist_locs[i] = maxIndex;
		min_diff[i] = centers[maxIndex] - x[i];
	}else if((centers[index] - x[i]) < (x[i] - centers[index-1])){
		min_dist_locs[i] = index;
		min_diff[i] = centers[index] - x[i];
	}else{
		min_dist_locs[i] = index-1;
		min_diff[i] = centers[index-1] - x[i];
	}

	center_offsets[min_dist_locs[i]] += min_diff[i];
	cluster_sizes[min_dist_locs[i]] += 1;


	//update centers
	//note: we can be sure that centers stay sorted as they are updated.
	for(i = 0; i < num_centers; i++){
		if(cluster_sizes[i] != 0){ //check if center has no points in its cluster. unlikely, but possible if we go a large number of rows without full update
			center_offsets[i] /= cluster_sizes[i];
		}
		centers[i] -= center_offsets[i];
	}

	//adjust dists to new centers
	for(i = 0; i < length_x; ++i){
		min_diff[i] -= center_offsets[min_dist_locs[i]];
	}
}

float RealGaussEval(float* x, float* y, int length_x, int length_y,
		    float sigma){
	//simple implementation of gauss transform, for reference/testing. 

	//note, in matts pdf, the denominators are WRONG! 
	//in the FGT paper he references, eq(7) has the equation with correct denominators
	float denom = 1/(4 * sigma * sigma);

	float out,temp;
	int i,j;
	out = 0;
	for (i=0;i<length_x;i++){
		for (j=0;j<length_y;j++){
			temp = x[i]-y[j];
			out += expf(-1.0f * temp * temp * denom);
		}
	}
	return out;
}

#define M_1_SQRT2PI 0.3989422804f
float calcPSMEntryContrib(float* x, int winSize, float sigma)
{
	//computes the correct PMS calculation, as done in actual app.
	//note: im 99% sure this function is correct, but maybe matt should double check juuust in case.
	//This is just here to test approximation for correctness, and compare speed.
	int i, j;
	float out, temp;
	out = 0;
	for (i = 0; i < winSize; i++){
		for (j = 1; j <= winSize; j++){
			temp = x[i] - x[i+j]; 
			out += expf(-1.0f * temp * temp);
		}
	}
	out *= M_1_SQRT2PI / sigma;
	return out;
}

float PSM_Gauss_Real(float* x, int winSize, float sigma)
{
	//a simple test example of calculating PMS using the gauss tranform.
	//just for testing.
	int i;
	float out;
	out = 0;
	for (i = 0; i < winSize; i++){
		//instead of passing sigma to RealGaussEval, we pass 0.5f.
		//this is so the denom becomes 1, to match PSM.
		out += RealGaussEval(&x[i], &x[i+1], 1, winSize, 0.5f);
	}
	out *= M_1_SQRT2PI / sigma;
	return out;
}

float PSM_Gauss_Fast_Variable(float* arr, int winSize, float sigma)
{
	//an older variation where p is a variable and can be changed. kept for testing.
	int i, j, c, p, n, offset, num_centers, *min_dist_locs, length_x, length_y, *cluster_sizes;
	float out, z, temp, xj, *centers, *cluster_diffs, *min_diff, *x, *y, *center_offsets;
	p = 1; //increasing p has negligible affect on accuracy
	num_centers = 5; //can *probably* still be accurate enough with n=4
	out = 0;
	length_x = 1;
	length_y = winSize;

	centers = malloc(sizeof(float) * num_centers);
	min_diff = malloc(sizeof(float) * winSize);
	min_dist_locs = malloc(sizeof(int) * winSize);
	cluster_diffs = malloc(sizeof(float) * num_centers * p);
	center_offsets = malloc(sizeof(float) * num_centers);
	cluster_sizes = malloc(sizeof(int) * num_centers);

	for (i = 0; i < winSize; i++){
		//instead of passing sigma to FGT, we pass 0.5f.
		//this is so the denom becomes 1, to match PSM.
		x = &arr[i];
		y = &arr[i+1];

		// compute the centers, as well as the min_diff and min_dist_locs arrays
		//min_dist_locs[i] holds the index of the center closest to x[i]
		//min_diff[i] holds the distance between x[i] and its closest center
		if(i == 0){
			sorted_clustering(num_centers, centers, min_diff,
							 min_dist_locs, y,
							 length_y, center_offsets, cluster_sizes);
		}
		else{ //a faster sorting for updating centers on subsequent rows
			sorted_clustering_recompute(num_centers, centers, min_diff,
						 min_dist_locs, y,
						 length_y, center_offsets, cluster_sizes);
		}
		
		// compute cluster differences
		//cluster_diffs[i*p + j] = C(n,s), where n = j and s = i
		//sigma in FGT hardcoded to 0.5f so FGT properly emulates PSM
		compute_cluster_diffs(centers, cluster_diffs, num_centers,
				      min_dist_locs, min_diff, length_y,
				      0.5f, p);

		/* we might be able to get away with not storing the cluster_diffs if 
		 * we alter order in which we iterate over different terms */
		for (j=0; j<length_x; ++j){
			xj = x[j];
			for (c=0, offset = 0; c<num_centers; ++c, offset+=p)
			{	
				z = (xj-centers[c]);
				temp = cluster_diffs[offset];
				for(n = 1; n < p; ++n){
					temp += (fgtComponent(n,z) * cluster_diffs[offset + n]); //this is just 1/n! * h(n,x)
					//temp += (1/fact(n)) * hermite_function(n,z) * cluster_diffs[offset + n];
				}
				out+= exp(-z*z) * temp;
				//out += temp;
			}
		}
	}
	out *= M_1_SQRT2PI / sigma;

	free(centers);
	free(min_diff);
	free(min_dist_locs);
	free(cluster_diffs);
	free(center_offsets);
	free(cluster_sizes);

	return out;
}

float PSM_Gauss_Fast(float* arr, int winSize, float sigma)
{
	//The actual alg! computing PMS calculation using FGT.
	//this implementation hardcodes p to be 1, and takes advantage of this.
	//num_centers is still not hardcoded.
	//still currently rather slow, slower than normal PSM calculation.
	//this is due to slowness of finding centers and cluster_diffs for each row.
	//we acheive some speedup by using a faster function after the first row to update centers.
	//there is still much room for improvement, and i have several ideas which will help quite a bit.
	//further means of improvement listed below:
	
	//for starters, having p=1 is quite helpful for computing cluster diffs.
	//when p=1, cluster_diffs[i] is just an int of how many points are part of cluster[i].
	//I am already calculating this value in sorted_clustering_recompute as cluster_sizes, 
	//so compute_cluster_diffs can and should be removed in its entirety when p is hardcoded to 1.

	//next oprimization, sorted_clustering_recompute is still far too slow.
	//should be taking advantage of fact that only one point changes when we advance rows.
	//we could try just doing a 'partial update'.
	//leave centers unchanged, leave old values for min_diffs and min_dist_locs, and just add new point.
	//would retain good enough accuracy, at least for a few rows. We could then do a 'full update' (aka sorted_clustering_recompute) every 'x' rows.

	//to make the 'partial updates' faster, make min_dist_locs and min_diff have length of length_x * 2!
	//then, when updating row, just advance pointer at each array forward one index. they are updated! just have to find the last index!

	//still am not using the fast expf() approcimation function. needs to be tested more! 

	//last, but definitly not least, Im pretty sure the value calculated for every 'row' in PSM could be saved and reused for the next hop.
	//this means for subsequent hops, only 55 rows (the hopsize) would need to be calculated, rather than 137!!!
	//im *pretty* sure this is true? bc sigma is different, but is only added in at the final step.
	int i, j, c, n, offset, num_centers, *min_dist_locs, length_x, length_y, *cluster_diffs;
	float out, z, temp, xj, *centers, *min_diff, *x, *y, *center_offsets;
	num_centers = 5; //can *probably* still be accurate enough with n=4
	out = 0;
	length_x = 1;
	length_y = winSize;

	centers = malloc(sizeof(float) * num_centers);
	min_diff = malloc(sizeof(float) * winSize);
	min_dist_locs = malloc(sizeof(int) * winSize);
	cluster_diffs = malloc(sizeof(int) * num_centers);
	center_offsets = malloc(sizeof(float) * num_centers);

	for (i = 0; i < winSize; i++){
		x = &arr[i];
		y = &arr[i+1];

		// compute the centers, as well as cluster_diffs
		//min_dist_locs[i] holds the index of the center closest to x[i]
		//min_diff[i] holds the distance between x[i] and its closest center
		if(i == 0){
			sorted_clustering(num_centers, centers, min_diff,
							 min_dist_locs, y,
							 length_y, center_offsets, cluster_diffs);
		}
		else{ //a faster sorting for updating centers on subsequent rows
			sorted_clustering_recompute(num_centers, centers, min_diff,
						 min_dist_locs, y,
						 length_y, center_offsets, cluster_diffs);
		}

		for (j=0; j<length_x; ++j){
			xj = x[j];
			for (c=0; c<num_centers; ++c)
			{	
				z = (xj-centers[c]);
				out+= exp(-z*z) * cluster_diffs[c];
			}
		}
	}
	out *= M_1_SQRT2PI / sigma;

	free(centers);
	free(min_diff);
	free(min_dist_locs);
	free(cluster_diffs);
	free(center_offsets);

	return out;
}

float uniform_rand_inclusive(){
	// returns a random number with uniform distribution in [0,1]
	return ((float)rand() - 1.0)/((float)RAND_MAX-2.0);
}

void PSMTEST()
{
	int length_x = 137;
	float* x = malloc(sizeof(float)*length_x*2);
	float sigma = 1;
	int r = 1500;
	int i;
	float elapsed, result;
	clock_t c1, c2;

	srand(87956);
	for (int i = 0; i < (length_x*2); i++){
		x[i] = uniform_rand_inclusive()*2;
	}

	for(int j = 0; j < 5; j++){
		c1 = clock();
		for(i = 0; i < r; i++){
			result = calcPSMEntryContrib(x, length_x, sigma);
		}	
		c2 = clock();
		elapsed = ((float)(c2-c1))/CLOCKS_PER_SEC;
		printf("PSMEntryContrib        : %f\t  time: %f\n", result, ((elapsed*1000)/r));

		c1 = clock();
		for(i = 0; i < r; i++){
			result = PSM_Gauss_Real(x, length_x, sigma);
		}	
		c2 = clock();
		elapsed = ((float)(c2-c1))/CLOCKS_PER_SEC;
		printf("PSM_Gauss_Real         : %f\t  time: %f\n", result, ((elapsed*1000)/r));

		c1 = clock();
		for(i = 0; i < r; i++){
			result = PSM_Gauss_Fast_Variable(x, length_x, sigma);
		}	
		c2 = clock();
		elapsed = ((float)(c2-c1))/CLOCKS_PER_SEC;
		printf("PSM_Gauss_Fast_Variable: %f\t  time: %f\n", result, ((elapsed*1000)/r));

		c1 = clock();
		for(i = 0; i < r; i++){
			result = PSM_Gauss_Fast(x, length_x, sigma);
		}	
		c2 = clock();
		elapsed = ((float)(c2-c1))/CLOCKS_PER_SEC;
		printf("PSM_Gauss_Fast         : %f\t  time: %f\n", result, ((elapsed*1000)/r));

		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	PSMTEST();

	return 1;
}
