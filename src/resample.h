/// @file     resample.h
/// @brief    Declaration of resampling functions

/// Performs resampling by calling libsamplerate
/// @param[in]  input The input audio stream that is to be resampled
/// @param[in]  len The length of input
/// @param[in]  sampleRatio Factor by which the sample rate increases
/// @param[out] output The array to which the output is written. This is
///     assumed to have a length given by `ResampledLength`
/// @returns The actual length of the output. Negative values encode errors
int Resample(const float * input, int len, float sampleRatio, float *output);

/// Resamples audio and allocates the output
/// @param[in] The input audio stream that is to be resampled
/// @param[in]  len The length of input
/// @param[in]  sampleRatio Factor by which the sample rate increases
/// @param[out] output This is a pointer that will be modified to point to the
///     location in memory that this function allocates and uses to store the
///     resampled audio. If this initially points to a separate, already
///     allocated location in memory, it will NOT be freed
/// @returns The length of the output. Negative values encode errors.
///     This will not return 0.
///
/// @note
/// This function wraps libsamplerate. Since the details of that library are
/// somewhat opaque, slightly more memory may be allocated than necessary
int ResampleAndAlloc(const float * input, int len, float sampleRatio,
		     float **output);


/// Helper function that estimates the length of resampled audio
/// @param[in] the input length
/// @param[in] sampleRatio Factor by which the sample rate increases
/// @returns The estimated length of the resampled audio. Negative values
///     encode errors. This will not return 0.
///
/// @note
/// This is only exposed for the purposes of documenting the python bindings
int ResampledLength(int len, float sampleRatio);


