/// @file     gammatoneFilter.h
/// @brief    [\ref transient] Declaration of the gammatone filter.

/// @ingroup transient
/// Performs IIR filtering using a cascade of second-order sections of biquad
/// filters
/// @param[in]     num_stages the number of biquad filters to apply. Must be at
///     least 1
/// @param[in]     coef a num_stages * 6 entry array with the feedforward and
///     feedback coefficients for every stage of filtering.
/// @param[in]     x the input passed through the filter.
/// @param[out]    y the output, it must already be allocated and have the same
///     length as x.
/// @param[in]     length the length of both x and y
/// @param[in,out] state Optional array parameter used to track intermediate
///     state. This allows for processing `x` in chunks and computing output
///     values that are identical to the outputs that would be computed if `x`
///     were processed in a single shot. This array is expected to have
///     `2*num_stages`. Alternatively, if the entire signal is being passed to
///     this function at once, `NULL` can be passed to this argument. In that
///     case, `state` is treated as an array of zeros (which assumes silence
///     preceeded the recording)
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
int sosFilter(int num_stages, const double* coef, const float* x, float* y,
	      int length, double* state);

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
void sosGammatoneCoef(float centralFreq, int samplerate, double* coef);

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
/// @param[in,out] state Optional array parameter used for processing `input`
///     in chunks. To process `input` in chunks, the same pointer must be
///     passed to each function call without modifying the underlying data.
///     In this case, state should be an array with 8 elements. Before
///     processing the very first chunk, the entries should be initialized to
///     0. If only a single chunk is processed, this can just be NULL.
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
		 int samplerate, int datalen, double* state);

/// @ingroup transient
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
void centralFreqMapper(size_t numChannels, float minFreq, float maxFreq,
		       float* fcArray);
