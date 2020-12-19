/// @file     calcSummedLagCorrentrograms.h
/// @brief    Declaration of calcSummedLagCorrentrograms

/// Helper function that checks that the argument properties meet the
/// requirements for the algorithm
///
/// see the documentation for CalcSummedLagCorrentrograms for a description of
/// the arguments
int CheckCorrentrogramsProp(size_t winsize, size_t max_lag, size_t hopsize);

/// Computes a series of "autocorrentrograms" and then reports the sums over
/// all of the lags in each autocorrentrogram. This implementation uses a fast
/// approximation for a Gaussian kernel.
///
/// The individual autocorrentrograms are analogous to the individual fourier
/// transforms in a short time fourier transform. An autocorrentrogram is
/// similar to an autocorrelogram except that the sum of multiplications
/// between the signal and itself with some lag (autocorrelation) are replaced
/// by a window of kernel evaluations operating on the difference between the
/// signal and itself after some lag.
///
/// @param[in]  x              Signal from which the autocorrentrograms are
///    computed. It must contain `(n_win-1)*hopsize + winsize + max_lag`
///    entries.
/// @param[in]  bandwidths     Holds the kernel bandwidth used for each
///    hopsize. It must contain `n_win` entries.
/// @param[in]  winsize        The number of entries per window. This must be
///    greater than or equal to 1.
/// @param[in]  max_lag        The maximum lag used when computing the
///    autocorrentropy for applied between the `x` and itself. This be no
///    smaller than the minimum lag, which is 1.
/// @param[in]  hopsize       The number of samples in `x` separating each
///    section of `x` that is used to compute a autocorrentrogram.
/// @param[in]  n_win          The number of acgrams to compute.
/// @param[out] summed_acgrams This is the array where the calculated
///     autocorrentrograms are stored. This should have n_win entries. The
///     newly calculated values are in-place to the initial values already
///     stored.
///
/// @return zero upon success
///
/// @note
/// For optimization-related purposes, `x`, and `summed_acgrams` must both have
/// have 16-byte memory alignment. None of the arrays can overlap.
/// Additionally, `winsize`, `max_lag`, and `hopsize` must all be integral
/// multiples of 4.
///
/// @par Mathematical description
/// In mathematical notation, the calculation can be cast as:
/// summed_acgrams[i] +=
///    OuterSum( InnerSum( K(x[i*hopsize + n] - x[i*hopsize + n + (j+1)],
///                          bandwidths[i] ) ) ) / winsize.
/// We note that
///     - OuterSum increments n = 0 through n = winsize - 1
///     - InnerSum j = 1 through j = max_lag
///     - K(u,b) is a kernel function with bandwidth b. For this calculation,
///       we define it as a normalized Gaussian curve, with mean = zero and
///       standard deviation = b, that is evaluated for some value u.
int CalcSummedLagCorrentrograms(const float * restrict x,
				const float * restrict bandwidths,
				size_t winsize, size_t max_lag,
				size_t hopsize, size_t n_win,
				float * restrict summed_acgrams);
