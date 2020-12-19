/// A quick and dirty implementation of detection function calculation
///
/// The implementation is not memory efficient and sigma optimization can
/// definitely be improved to be much faster
///
/// @param[in] correntropyWinSize The window size for the calculation of
///            correntropy (specified as a number of audio frames). In this
///            implementation, the maximum lag value is also given by this
///            value. The paper suggests a value of samplerate/80 if
///            minFreq = 80 Hz
/// @param[in] interval The hopsize, h, used in calculation of detection
///            function (specified as a number of audio frames). This is
///            assumed to be the same as the interval h for optimizing sigma.
///            The paper suggests a value of 5ms (~55 samples when sampleRate
///            is 11025 Hz)
/// @param[in] scaleFactor The constant used to estimate the optimized sigma.
///            For Silverman's rule of thumb, this is (4./3.)^(1/5)
/// @param[in] sigWindowSize The size of the window used to compute the
///            optimized sigma for the kernel (specified as a number of audio
///            frames). The paper suggests a window size of 7s (7*sampleRate).
/// @param[in] numChannels The number of channels in the FilterBank. This must
///            be positive. The paper suggests a value of 64. If set to 1, then
///            minFreq and maxFreq must be equal; the filterbank will have 1
///            channel with central frequency equal to minFreq and maxFreq.
/// @param[in] minFreq The lowest frequency (in Hz) included in the bandwidth
///            of a channel of the filterbank. The paper suggests 80 Hz.
/// @param[in] maxFreq The maximum frequency (in Hz) included in the bandwidth
///            of a channel of the filterbank. The paper suggests 4000 Hz.
/// @param[in] sampleRate The samplerate of the audio data passed to data.
/// @param[in] dataLength The length of the input audio data.
/// @param[in] data The input audio data for which the detection function
///            should be computed.
/// @param[in] detFunctionLength The expected length of the data function
/// @param[out] detFunction An array that will be modified to have values of
///             the detection function
///
/// @return Returns 0 for success and -1 for failure.
///
/// @par Implementation Notes:
/// We made some assumptions about parameters mentioned in the paper:
///    1. We assume that the interval h for the sigmaOptimizer is the same as 
///       the hopsize h used when calculating the detection Function. In this 
///       implementation we refer to this value as the interval.
///    2. We assume that the correntropy window size is always equal to the 
///       maximum lag value.
///
/// @par Note about terminology:
///    In the paper they talk about pooling all correntropy values calculated
///    for different channels with constant lags and into the summary matrix.
///    Then when they compute the detection function at t, they subtract the
///    summary matrix summed over all lags at t + h from the summary matrix 
///    summed over all lags at t. For the purpose of our implementation, we 
///    have named the summary matrix summed over all lags, the pooled Summary 
///    Matrix.
int simpleDetFunctionCalculation(int correntropyWinSize, int interval,
				 float scaleFactor, int sigWindowSize,
				 int numChannels, float minFreq, float maxFreq,
				 int sampleRate, int dataLength, float* data,
				 int detFunctionLength, float* detFunction);

/// Computes the length of the resulting detection function
int computeDetFunctionLength(int dataLength, int correntropyWinSize,
			     int interval);

// Helper Functions:

/// Computes the rolling optimized sigma to be used by the kernel functions 
///
/// This function is implemented with a rolling window and the implementation
/// is adapted/inspired by the implementation of the Rolling.std function of
/// the pandas Python package
///
/// @param[in] startIndex The index in buffer where the first window used to
///            compute sigma is centered.
/// @param[in] interval The number of indices that the window should be
///            advanced to compute the next sigma.
/// @param[in] scaleFactor The constant scale factor used to compute the sigma
///            for the kernel. For Silverman's rule of thumb, this is
///            (4./3.)^(1/5)
/// @param[in] sigWindowSize The length of the rolling window
/// @param[in] dataLength The length of the input window
/// @param[in] numWindows The number of times that the value should be computed
///            for the given input stream and the length of `sigmas`.
///            Typically, this is 1 larger than the value returned by
///            `computeDetFunctionLength`
/// @param[in] buffer The buffer of data for which the rolling sigma is
///            computed
/// @param[out] sigmas The buffer of data that the results get written into
///
/// @par Implementation Notes:
/// For values of sigma near the start and of buffer, the windows are
/// automatically adjusted not to include values beyond the edge of the array.
/// At these points, the number of values used to compute sigma are just
/// reduced.
///
/// @par
/// The current implementation allows the center of the window to extend past
/// the end of the array.
///
/// @par
/// For a window centered at index `i`, the final element included in the 
/// window is always located at `min(dataLength,i+sigWindowSize//2)`. If
/// `sigWindowSize` is even, then the first element included in the window 
/// is at `max(0,i-sigWindowSize//2 -1)`. Otherwise, the first element is 
/// located at `max(0,i-sigWindowSize//2)`.
///
/// @par
/// The current implementation casts the single precision input data to double
/// precision during the calculation and then casts back at the end.
///
/// @par General Note:
/// In general, rollSigma with Silverman's rule of thumb is optimized to be
/// used for KDE with Gaussian Kernel functions when trying to approximate a
/// probability distribution function resembling a Gaussian. (If the
/// is not univariate or different kernel functions are used, it is advisable
/// to try to optimize the bandwidth of each kernel in a different manner)
void rollSigma(int startIndex, int interval, float scaleFactor,
	       int sigWindowSize, int dataLength, int numWindows,
	       float *buffer, float *sigmas);


/// Iterates over the input from a given channel and computes Pooled Summary
/// Contribution for that channel (the sum of of correntropy over all lags)
///
/// The calculation computes correntropy using Gaussian Kernels
///
/// @param[in] correntropyWinSize The window size for the calculation of
///            correntropy (specified as a number of audio frames). In this
///            implementation, the maximum lag value is also given by this
///            value.
/// @param[in] interval The hopsize, h, is the number of indices to advance
///            over in `buffer` between each calculation of the pooled summary
///            matrix contribution. The paper suggests a value of 5ms (~55
///            samples when sampleRate is 11025 Hz)
/// @param[in] numWindows The number of times that the pooled summary matrix
///            contribution should be computed from `buffer`. Typically, this
///            is 1 larger what is returned by `computeDetFunctionLength`
/// @param[in] buffer The input (filtered) signal for which the pooled summary
///            matrix calculation is performed. This should have a length of at
///            least `(numWindows-1)*interval + 2*correntropyWinSize + 2`
/// @param[in] sigmas The array of sigmas to use to compute the pooled summary
///            matrix contribution at each interval. This should have
///            `numWindows` entries
/// @param[out] pSMatrix The array that the pooled summary matrix contributions
///             are added to. This should have a length of at least
///             `numWindows`.
///
/// @par Notes:
/// The idea behind this function was to initialize `pSMatrix` ahead of time as
/// an array of zeros and then repeatedly call this function (but passing
/// different values to `buffer` and `sigmas` that correspond to the different
/// channels).
void pSMContribution(int correntropyWinSize, int interval, int numWindows,
		     float *buffer, float *sigmas, float *pSMatrix);
