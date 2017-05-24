#include <stdlib.h>
#include "findpeaks.h"
#include "findCandidates.h"
#include "candidateSelection.h"
#include "candidates.h"
#include "lists.h"
#include "BaNaDetection.h"


// here we define the actual BaNa Fundamental pitch detection algorithm
// NOTE: in all of my code I use window, frame, and block to refer to the same
//       thing

double* BaNa(double **AudioData, int size, int dftBlocksize, int p,
	     double f0Min, double f0Max, double* frequencies){
  // Implements the BaNa fundamental pitch detection algorithm
  // This algorithm is split into 3 parts:

  // 1. Preprocessing
  //    - mask all frequencies outside of [f0Min, p * f0Max]
  // 2. Determination of F0 candidates. For each frame:
  //    a. Retrieve p harmonic peaks - for ordinary BaNa algorithm, these are
  //       the p peaks with lowest frequencies
  //    b. Calculate F0 candidates from harmonic peaks
  //    c. Add F0 found by Cepstrum to the F0 candidates
  //    d. Add harmonic peak with lowest frequency to F0 candidates
  // 3. Post-Processing. Selection of F0 from candidates


  double *fundamentals;
  struct candidateList **windowCandidates;
  int numBlocks = size / dftBlocksize;
  long i;
  // preprocess each of the frames
  BaNaPreprocessing(AudioData, size, dftBlocksize, p, f0Min, f0Max,frequencies);

  // find the candidates for the fundamentals
  BaNaFindCandidates(AudioData, size, dftBlocksize, p, f0Min, f0Max, 0,
		     windowCandidates,10.0,frequencies);

  // determine which candidate is the fundamental
  fundamentals = candidateSelection(windowCandidates, numBlocks);

  // clean up
  for (i=0;i<numBlocks;i++){
    candidateListDestroy(windowCandidates[i]);
  }
  
  free(windowCandidates);
  return fundamentals;
}

double* BaNaMusic(double **AudioData, int size, int dftBlocksize, int p,
		  double f0Min, double f0Max, double* frequencies){
  // Same as BaNa except that during determination of F0 candidates, retrieve
  // the p peaks with the maximum amplitude

  double *fundamentals;
  struct candidateList **windowCandidates;
  long numBlocks = size / dftBlocksize;
  long i;
  // preprocess each of the frames
  BaNaPreprocessing(AudioData, size, dftBlocksize, p, f0Min, f0Max,frequencies);

  // find the candidates for the fundamentals
  BaNaFindCandidates(AudioData, size, dftBlocksize, p, f0Min, f0Max, 0,
		     windowCandidates,3.0,frequencies);

  // determine which candidate is the fundamental
  fundamentals = candidateSelection(windowCandidates, numBlocks);

  // clean up
  for (i=0;i<numBlocks;i++){
    candidateListDestroy(windowCandidates[i]);
  }
  
  free(windowCandidates);
  return fundamentals;
}



void BaNaPreprocessing(double **AudioData, int size, int dftBlocksize, int p,
		       double f0Min, double f0Max, double* frequencies){
  // set all frequencies outside of [f0Min, p * f0Max] to zero
  int blockstart, i;
  double temp;
  double maxFreq = (((double)p)*f0Max);
  for (blockstart = 0; blockstart < (size - dftBlocksize); blockstart += dftBlocksize){
    for (i=0; i<dftBlocksize; i++){
      // use a better algorithm
      // for now this is a placeholder
      temp = frequencies[i];
      if ((temp<f0Min)||(temp>maxFreq)){
	(*AudioData)[blockstart + i] = 0;
      }
    }
  }
}

void BaNaFindCandidates(double **AudioData,int size,int dftBlocksize, int p,
			double f0Min, double f0Max, int first,
			struct candidateList **windowCandidates,
			double xi, double* frequencies){
  // this finds all of the f0 candiates
  // windowCandidates is an array of pointers that point to the list of
  // candidates for each window

  
  int i;
  int numBlocks = size / dftBlocksize;
  int blockstart;
  int numPeaks;
  double *magnitudes = malloc(dftBlocksize * sizeof(double));
  double temp, slopeThreshold, ampThreshold,smoothwidth;
  double *peakFreq, *peakMag, *measuredWidth;
  long *peakIndices;
  
  struct orderedList candidates;
  
  windowCandidates = malloc(sizeof(struct candidateList*)*numBlocks);

  // outer loop iterates over blocks
  for (blockstart = 0; blockstart < (size - dftBlocksize); blockstart += dftBlocksize){
    
    // for now copy the magnitudes into a buffer. Fix this in the future
    for(i = 0; i < dftBlocksize; ++i){
      magnitudes[i] = (*AudioData)[blockstart + i];
    }

    // determine slopeThreshold, ampThreshold, smoothwidth
    // set ampThreshold to 1/15 th of largest magnitude
    ampThreshold = magnitudes[0];
    for (i=1; i < dftBlocksize; ++i){
      temp = magnitudes[i];
      if (temp>ampThreshold){
	ampThreshold = temp;
      }
    }
    ampThreshold/=15.;
    
    // not sure what to set these variables values to. For now, arbitrary
    // values
    smoothwidth = 5;
    slopeThreshold = 0;
    

    
    // find the harmonic spectra peaks
    numPeaks = findpeaks(frequencies, magnitudes, (long)dftBlocksize,
			 slopeThreshold, ampThreshold, smoothwidth, 5, 3, 
			 p, first, peakIndices,peakFreq, peakMag, 
			 measuredWidth);

    // determine the candidates from the peaks
    candidates = calcCandidates(peakFreq, numPeaks);
    // add the lowest frequency peak fundamental candidate
    // add the cepstrum fundamental candidate

    // determine the distinctive candidates and add them to windowCandidates[i]
    windowCandidates[i] = distinctCandidates(&candidates,(p-1)*(p-1)+2, xi);
    
    orderedListDestroy(candidates);

    free(peakFreq);
    free(peakMag);
    free(measuredWidth);
    free(peakIndices); 
  }
free(magnitudes);

}

int main(int argc, char*argv[]){
  // only including this for testing purposes
  return 0;
}