#include "../lists.h"

/// Internal helper function that identifies transients (onsets and offsets)
/// from the provided detection function
///
/// @param[in]  detection_func Array of detection function values. This will be
///             modified, in-place.
/// @param[in]  len Number of values in detection_func
/// @param[out] transients A pointer to an initially empty intList that will be
///             filled with integers that indicate the interval in the 
///             detection function in which a transient has been identified.
///             The transients alternate between being onsets and offsets. The
///             first transient is always an onset while the last transient is
///             always on offset (there are always an even number of values).
///
/// @return Returns the length of transients. A value of 0 or smaller indicates
///         that an error occured
///
/// Note: We added an extra step not detailed in the paper in which we
/// normalized the detection such that it has maximum magnitude of 1.
///
/// Note: We added another extra step in which we dropped the last pair of
/// detected transients (the algorithm almost always return an extra false
/// positive note at the end)
int SelectTransients(float* detection_func, int len, intList* transients);
