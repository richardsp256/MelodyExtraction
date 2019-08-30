#include "gammatoneFilter.h"

/// Computes the central frequency used by the gammatone filter for each
/// channel. The frequencies are mapped according to the Equivalent Rectangular
/// Bandwidth (ERB) scale.
///
/// @param[in] numChannels The number of channels in the FilterBank. This must
///            be positive. The paper suggests a value of 64. If set to 1, then
///            minFreq and maxFreq must be equal; the filterbank will have 1
///            channel with central frequency equal to minFreq and maxFreq.
/// @param[in] minFreq The lowest frequency (in Hz) included in the bandwidth
///            of a channel of the filterbank. The paper suggests 80 Hz.
/// @param[in] maxFreq The maximum frequency (in Hz) included in the bandwidth
///            of a channel of the filterbank. The paper suggests 4000 Hz.
/// @param[in] fcArray A preallocated array of length numChannels where the
///            computed central frequencies will be stored.
///
/// @par Notes
/// A linear approximation of ERB between 100 and 10000 Hz, from wikipedia
/// ( https://en.wikipedia.org/wiki/Equivalent_rectangular_bandwidth ),
/// is given by ERB = 24.7 * (0.00437 * f + 1). Both ERB and f are in units of
/// Hz. We use this approximation since it's the same as the one recommended
/// for use in the paper detailing our gammaton implementation. According to
/// that paper, the ERB scale (ERBS) for the linear approximation is 
/// ERBS = 21.3 * log10(1+0.00437*f). We can invert the equation to get
/// f = (10^(ERBS/21.4) - 1)/0.00437
///
/// @par
/// In general, the minimum and maximum frequencies (fmin and fmax) included in
/// a bandwidth B, centred on fc are: fmin = fc - B/2 and fmax = fc + B/2. If
/// there are at least 2 channels, the minimum and maximum central frequencies
/// are selected so that there bandwidths are just barely `minFreq` and
/// `maxFreq`. In other words: minFreq = min(fcArray) - B/2 and
/// maxFreq = max(fcArray) + B/2. If we plug our equation for ERB into each
/// equation, we can solve for min(fcArray) and max(fcArray):
///     min(fcArray) = (minFreq + 12.35)/0.9460305
///     max(fcArray) = (maxFreq - 12.35)/1.0539695
///
/// @par
/// We will space out the remaining central frequencies with respect to EBRS
/// We define minERBS = ERBS(min(fcArray)) and maxERBS = ERBS(max(fcArray)).
/// The ith entry of fcArray will have an EBRS given by
/// ERBS(fcArray[i]) = minERBS + i * (maxERBS-minERBS)/(numChannels - 1)
/// Finally, we can invert the formula for ERBS to get the frequency of each
/// entry: fcArray[i] =(10.^((minERBS + i * (maxERBS-minERBS)
int centralFreqMapper(int numChannels, float minFreq, float maxFreq,
		      float* fcArray);

// The remainder of this header file was written with the intention of being
// used to process chunks. This functionallity does not currently exist (the
// following could probably be excised from the main development branch)


// Had not originally thought that I would need to individually allocate
// memory for the pointer to every member of channelData (it's obvious now).
// Need to modify implementation

struct filterBank{
	
	struct channelData* cDArray;
	int numChannels; 
	int lenChannels; // in units of number of samples
	int overlap; // in units of number of samples
	int samplerate;
};


struct channelData{
	float cf; // center frequency
};

struct filterBank* filterBankNew(int numChannels, int lenChannels, int overlap,
				 int samplerate, float minFreq, float maxFreq);

void filterBankDestroy(struct filterBank* fB);

/* The following 3 functions are used for processing inputChunks. The first 
 * function is used when you feed in the very first chunk. The second function
 * is used when you feed in all other chunks except the last chunk. The third 
 * function is used when you feed in the last chunk 
 *
 * for all functions, nsamples is the number of samples in the inputChunk
 */

void filterBankFirstChunk(struct filterBank* fB, float* inputChunk,
			  int nsamples, float** leadingSpectraChunk);

void filterBankUpdateChunk(struct filterBank* fB, float* inputChunk,
			   int nsamples, float** leadingSpectraChunk,
			   float** trailingSpectraChunk);

void filterBankFinalChunk(struct filterBank* fB, float* inputChunk,
			  int nsamples, float** leadingSpectraChunk,
			  float** trailingSpectraChunk);


