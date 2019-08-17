#include "lists.h"

//might make sense to rename the file

/// Internal helper function that identifies transients from the provided
/// detection function
///
/// \param[in] detection_func Array of detection function values.
/// \param[in] len Number of values in detection_func
/// \param[out] transients A pointer to an initially empty intList that will be
///             filled with integers that indicate the interval in the 
///             detection function in which a transient has been identified.
///             The transients alternate between being onsets and offsets. The
///             first transient is always an onset while the last transient is
///             always on offset (there are always an even number of values).
///
/// \return Returns the length of transients. A value of 0 or smaller indicates
///         that an error occured
///
/// Note: We added an extra step not detailed in the paper in which we
/// normalized the detection such that it has maximum magnitude of 1.
int detectTransients(float* detection_func, int len, intList* transients);

/// Identifies pairs of onsets and offsets from audio data using the algorithm
/// published by Chang & Lee (2016)
///
/// \param[in] audioData The audio data for which the transients should be
///            identified
/// \param[in] size Length of audioData
/// \param[in] samplerate The sample rate of the audio data (in Hz)
/// \param[out] transients A pointer to an initially empty intList. This list
///             will be filled with integers corresponding to the audio frame
///             where transients occured. The transients alternate between
///             onsets and offsets. The first value is always an onset while
///             the last value is always an offset.
///
/// Note: This uses the default settings mentioned in the method paper
int pairwiseTransientDetection(float *audioData, int size, int samplerate,
			       intList* transients);
