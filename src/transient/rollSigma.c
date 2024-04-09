#include <stdlib.h> // malloc, free
#include <math.h> //sqrtf
#include <stdbool.h>

#include "../errors.h"

// The following draws HEAVY inspiration of roll_var from pandas. Note, I can't
// remember the exact version of the code that this was derived from, but a
// version of it can be found here:
//   https://github.com/pandas-dev/pandas/blob/bd8dbf906e4352567094637c9c824c350dae3ad2/pandas/_libs/window.pyx
// The Pandas 3-clause BSD license is provided in licenses/PANDAS_LICENSE

struct windowIndexer{
	bool variable;
	bool centered;
	int sizeLeft;
	int sizeRight;
	int interval;
	int curLocation;
	int arrayLength;
};


int windowIndexerInit(bool variable, bool centered, int winSize, int interval,
		      int startIndex, int arrayLength,
		      struct windowIndexer* wInd)
{
	if (winSize <= 0){
		return ME_NONPOS_ROLLING_WINDOW;
	} else if (interval <= 0){
		return ME_NONPOS_ROLLING_INTERVAL;
	} else if (startIndex < 0){
		return ME_NEGATIVE_ROLLING_STARTINDEX;
	} else if (arrayLength <= 0){
		return ME_NEGATIVE_ROLLING_ARRLEN;
	}
	wInd->variable = variable;
	wInd->centered = centered;
	if (centered){
		wInd->sizeLeft = winSize/2;
		wInd->sizeRight = wInd->sizeLeft + 1;
		if (winSize % 2 == 0){
			(wInd->sizeLeft)-=1;
		}
	} else {
		wInd->sizeLeft = 0;
		wInd->sizeRight = winSize;
	}
	wInd->interval = interval;
	wInd->curLocation = startIndex;
	wInd->arrayLength = arrayLength;
	return ME_SUCCESS;
}

void wIndAdvance(struct windowIndexer *wInd){
	(wInd->curLocation) += (wInd->interval);
}

int wIndGetStart(struct windowIndexer *wInd){
	if (wInd->centered !=0){
		int temp;
		temp = wInd->curLocation - wInd->sizeLeft;
		if (temp < 0){
			// applies to both variable and non-variable windows
			return 0;
		}else{
			return temp;
		}
	} else {
		return (wInd->curLocation);
	}
}

int wIndGetStop(struct windowIndexer *wInd){
	int temp = wInd->curLocation + wInd->sizeRight;
	
	if (wInd->variable){
		if (temp > wInd->arrayLength){
			return wInd->arrayLength;
		} else {
			return temp;
		}
	} else {
		return temp;
	}
}




static inline double calc_var(double nobs, double ssqdm_x){
	double result;
	/* Variance is unchanged if no observation is added or removed
	 */
	if (nobs == 1){
		result = 0;
	} else {
		result = ssqdm_x / (nobs - 1);
		if (result < 0){
			result = 0;
		}
	}
	return result;
}

static inline void add_var(double val, int *nobs, double *mean_x,
			   double *ssqdm_x){
	/* add a value from the var calc */
	double delta;

	(*nobs) += 1;
	delta = (val - *mean_x);
	(*mean_x) += delta / *nobs;
	(*ssqdm_x) += (((*nobs-1) * delta * delta) / *nobs);
}

static inline void remove_var(double val, int *nobs, double *mean_x,
			      double *ssqdm_x){
	/* remove a value from the var calc */

	double delta;

	(*nobs) -= 1;
	if (*nobs>0){
		delta = (val - *mean_x);
		(*mean_x) -= delta / *nobs;
		(*ssqdm_x) -= (((*nobs+1) * delta * delta) / *nobs);
	} else {
		*mean_x = 0;
		*ssqdm_x = 0;
	}
}

/// Compute the rolling variance following the algorithm from the pandas python
/// package.
///
/// This algorithm can still be further optimized so that we add
/// and remove values from the windows in chunks (rather than 1 at a time).
int rollSigma(int startIndex, int interval, float scaleFactor,
	      int sigWindowSize, int dataLength, int numWindows,
	      float *buffer, float *sigmas)
{
	int i,j,k,nobs = 0,winStart, winStop, prevStop,prevStart;
	double mean_x = 0, ssqdm_x = 0;
	float std;
	struct windowIndexer wInd_instance;
	struct windowIndexer* wInd = &wInd_instance;

	int rv = windowIndexerInit(true, true, sigWindowSize, 1, startIndex,
				   dataLength, wInd);
	if (rv != ME_SUCCESS){
		return rv;
	}
	
	for (i=0;i<numWindows;i++){
		winStart = wIndGetStart(wInd);
		winStop = wIndGetStop(wInd);
		/* Over the first window observations can only be added 
		 * never removed */
		if (i == 0){
			for (j=winStart;j<winStop;j++){
				add_var((double)buffer[j], &nobs, &mean_x,
					&ssqdm_x);
			}
		} else {
			/* after the first window, observations can both be 
			 * added and removed */

			/* calculate adds 
			 * (almost always iterates over 1 value, for the final
			 *  windows, iterates over 0 values)
			 */
			for (j=prevStop;j<winStop;j++){
				add_var((double)buffer[j], &nobs, &mean_x,
					&ssqdm_x);
			}

			/* calculate deletes 
			 * (should always iterates over 1 value)
			 */
			for (j = prevStart; j<winStart; j++){
				remove_var((double)buffer[j], &nobs, &mean_x,
					   &ssqdm_x);
			}
		}
		wIndAdvance(wInd);
		prevStart = winStart;
		prevStop = winStop;

		/* compute sigma */
		std = sqrtf((float)calc_var(nobs, ssqdm_x));
		sigmas[i] = (scaleFactor*std/powf((float)nobs,0.2));

		/* Now we need to advance the window over the interval between  
		 * the current where we just calculated sigma and the next 
		 * location where we want sigma. 
		 * We do this because the windowIndexer only advances one index 
		 * at a time and we are only interested in keeping the sigma 
		 * values separated by interval (a number indices typically 
		 * greater than 1) */
		if (i == (numWindows-1)){
			break;
		}

		for (k = 1; k<interval;k++){
			winStart = wIndGetStart(wInd);
			winStop = wIndGetStop(wInd);
			/* after the first window, observations can both be 
			 * added and removed */

			/* calculate adds */
			for (j=prevStop;j<winStop;j++){
				add_var((double)buffer[j], &nobs, &mean_x, &ssqdm_x);
			}

			/* calculate deletes */
			for (j = prevStart; j< winStart;j++){
				remove_var((double)buffer[j], &nobs, &mean_x, &ssqdm_x);
			}
			wIndAdvance(wInd);
			prevStart = winStart;
			prevStop = winStop;
		}
	}
	return ME_SUCCESS;
}
