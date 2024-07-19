/// @file     calcDetFunc.h
/// @brief    [\ref transient] Declarataion of calcDetFunc

/// @ingroup transient
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
/// @param[in] audioLength The length of the input audio data.
/// @param[in] audio The input audio data for which the detection function
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
int CalcDetFunc(int correntropyWinSize, int interval, float scaleFactor,
		int sigWindowSize, int numChannels, float minFreq,
		float maxFreq, int sampleRate, int audioLength,
		const float* audio, int detFunctionLength, float* detFunction);

/// @ingroup transient
/// Computes the length of the resulting detection function
int computeDetFunctionLength(int dataLength, int correntropyWinSize,
			     int interval);
