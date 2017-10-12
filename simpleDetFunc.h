/* A quick and dirty implementation of detection function calculation
 *   - Not Memory efficient and sigmaOptimization can definitely be improved to 
 *     be much faster
 *
 * ==========
 * Arguments:
 * ==========
 *
 * correntropyWinSize - number of samples in the correntropy window. According  
 *                      to the paper, if minFreq = 80 Hz, this is usually 
 *                      samplerate/80. Note that the maximum lag is assumed to 
 *                      be equal to this value 
 * scaleFactor - used to compute sigma (according to 
 *               https://en.wikipedia.org/wiki/Kernel_density_estimation 
 *               Silverman's rule of thumb suggests (4/3)^0.2 ~1.06)
 * numChannels - number of channels in the FilterBank. This must be positive. 
 *               If this is set to 1, then minFreq and maxFreq must be equal. 
 *               Then the filterbank has 1 channel with central frequency equal 
 *               to minFreq and maxFreq.
 * minFreq - the lowest frequency included in the bandwidth of a channel of the 
 *           filterbank. Typically 80 Hz.
 * maxFreq - the highest frequency included in the bandwidth of a channel of 
 *           the filterbank. Typically 4000 Hz
 * sampleRate - sampling rate. Typically 11025 Hz
 * interval - hopsize, h, used in calculation of detection function. Assumed to 
 *            be the same as the interval h where sigma is optimized. Given in 
 *            number of samples. Paper suggests 5ms (~55 samples when 
 *            sampleRate is 11025 Hz)
 * sigWindowSize - window size for sigma optimizer in numbers of intervals. The 
 *                 paper suggests 7s (For a sampleRate of 11025 Hz and interval 
 *                 of 55, this is usually 1403).
 *
 * * ===========================
 * Notes about Implementation:
 * ===========================
 * We made some assumptions about parameters mentioned in the paper:
 *     1. We assume that the interval h for the sigmaOptimizer is the same as 
 *        the hopsize h used when calculating the detection Function. In this 
 *        implementation we refer to this value as the interval.
 *     2. We assume that the correntropy window size is always equal to the 
 *        maximum lag value.
 *
 * A note about terminology:
 *     In the paper they talk about pooling all correntropy values calculated 
 *     for different channels with constant lags and into the summary matrix. 
 *     Then when they compute the detection function at t, they subtract the 
 *     summary matrix summed over all lags at t + h from the summary matrix 
 *     summed over all lags at t. For the purpose of our implementation, we 
 *     have named the summary matrix summed over all lags, the pooled Summary 
 *     Matrix. 
 */
 

int simpleDetFunctionCalculation(int correntropyWinSize, int interval,
				 float scaleFactor, int sigWindowSize,
				 int sampleRate, int numChannels,
				 float minFreq, float maxFreq,
				 float* data, int dataLength,
				 float** detFunction)
