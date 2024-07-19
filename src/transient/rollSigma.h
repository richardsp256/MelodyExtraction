/// @file     rollSigma.h
/// @brief    [\ref transient] Declaration of the rollSigma function

/// @ingroup transient
/// Computes the rolling optimized sigma to be used by the kernel functions 
///
/// This function is implemented with a rolling window and the implementation
/// is adapted/inspired by the implementation of the `Rolling.std` function of
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
/// @returns Return 0 on success.
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
int rollSigma(int startIndex, int interval, float scaleFactor,
	      int sigWindowSize, int dataLength, int numWindows,
	      float *buffer, float *sigmas);
