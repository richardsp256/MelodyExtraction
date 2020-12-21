/// @file     gammatoneFilter.h
/// @brief    [\ref transient] Declaration of the gammatone filter.

/// @ingroup transient
/// Performs IIR filtering using a cascade of second-order sections of biquad
/// filters
/// @param[in]  num_stages the number of biquad filters to apply. Must be at
///     least 1
/// @param[in]  coef: a num_stages * 6 entry array with the feedforward and
///     feedback coefficients for every stage of filtering.
/// @param[in]  x: the input passed through the filter.
/// @param[out] y: the output, it must already be allocated and have the same
///     length as x.
/// @param[in] length: the length of both x and y
/// @returns 0 on success
///
/// @note
/// The coefficients of the ith stage should be at:
///     - `coef[(i*6)+0]= b0`, `coef[(i*6)+1] = b1`, `coef[(i*6)+2] = b2`
///     - `coef[(i*6)+3]= a0`, `coef[(i*6)+4] = a1`, `coef[(i*6)+5] = a2`
///
/// @par ToDo
/// To do list:
///    - Need to add an additional argument where state can be specified. To
///      make it ergonomic, we need to allow it to accept NULL
///    - Probably need to be checking for overflows and underflows - since this
///      is recursive it could screw up a lot.
///
/// @par Implementation Note
/// This implements the biquad filters using a Direct Form II Transposed
/// Structure (this structure seems to be fairly standard). The difference
/// equations are:
///     - `y[n] = (b0 * x[n] + d1[n-1])/a0`
///     - d1[n] = b1 * x[n] - a1 * y[n] + d2[n-1]
///     - d2[n] = b2 * x[n] - a2 * y[n]
int sosFilter(int num_stages, const double *coef, const float *x, float *y,
	      int length);

/// @ingroup transient
/// Computes the sos coefficients to implement a Gammatone filter
/// The algorithm is taken from Slaney 1993
/// https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf
///
/// @param[in]  centralFreq The central frequency (in Hz) of the Gammatone
///     filter
/// @param[in]  samplerate The sampling rate of the input that the filter will
///     be applied on
/// @param[out] coef Pre-allocated buffer used to store the 24 entries
void sosGammatoneCoef(float centralFreq, int samplerate, double *coef);

/// @ingroup transient
/// The sosGammatone is an IIR approximation for the gammatone filter.
///
/// The sos Gammatone Filter is described by Slaney (1993):
///    ( https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf )
/// Our entire implementation is based on his description (It is implemented 
/// using a cascade of 4 biquad filters or using 4 second order sections - sos).
///
/// @param[in]  input the input data to be filtered
/// @param[out] output an array to be filled with the output data
/// @param[in]  centralFreq The frequency of the gammatone Filter
/// @param[in]  samplerate The sampling rate of the input
/// @param[in]  datalen The number of entries in input
/// @returns 0 upon success
///
/// Although it is not currently implemented, I believe it is possible to set 
/// the coefficients of the cascaded biquad filter such that the response is 
/// normalized (the central frequency in the frequency response has fixed gain).
///
/// Implementation Notes:
/// - presently it recasts the input data as doubles for improved accuracy.
/// - The implementation should be adjusted to detect and actively avoid, 
///   overflows, underflows and NaNs (due to the recursive nature, the
///   appearance will spread throughout the output
///
/// @par Optimization Notes:
/// The current implementation has a long dependency chain. To get this to
/// vectorize we probably need to refactor this. But that doesn't seem
/// particularly necessary
///
/// @par Alternative Implementations
/// There may be some benefits to using a FIR filter instead.  
///
/// @par
/// Additionally, it is definitely worth considering an alternative IIR
/// approximatoin. The All-Pole Gammatone Filter (APGF) is also described by
/// Slaney (1993):
///   ( https://engineering.purdue.edu/~malcolm/apple/tr35/PattersonsEar.pdf )
/// and it should achieve better complexity. I believe additional discussion of
/// the APGF can be found in Lyon (1996):
///   ( http://www.dicklyon.com/tech/Hearing/APGF_Lyon_1996.pdf
/// Lyon (1996) also discuses how the One-Zero Gammatone Filter (OZGF) which is
/// slightly modified from the APGF and is supposed to be a slightly better
/// approximation for a gammatone.
int sosGammatone(const float* data, float* output, float centralFreq,
		 int samplerate, int datalen);



