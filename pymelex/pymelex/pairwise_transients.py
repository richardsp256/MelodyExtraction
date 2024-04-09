
import ctypes
from math import ceil

import numpy as np

from .shared_lib import libmelex
from .misc import \
    _check_audio, _check_1D_float_array, _ensure_int, _ensure_pos_int

from .IntList import IntList, intList_Struct

# currently this file is structured as follows:
#    1. First we provide wrappers around helper functions to allow for easy
#       experimentation.
#    2. Then we provide wrapper functions that wrap the full implementation of
#       of the pairwise transient detection method
# Long-term, this may be annoying to maintain

# wrap centralFreqMapper

_centralFreqMapper = libmelex.centralFreqMapper
_centralFreqMapper.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float,
                               np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                                      flags=('C_CONTIGUOUS',
                                                             'WRITEABLE'))]
_centralFreqMapper.restype = None
def central_freq_mapper(num_channels = 64, min_freq = 80., max_freq=4000.):
    """
    Computes the central frequency used by the gammatone filter for each
    channel. The frequencies are mapped according to the Equivalent Rectangular
    Bandwifth (ERB) scale.

    Parameters
    ----------
    num_channels : int, optional
        Number of channels in gammatone filterbank. The suggested value by the 
        paper is 64 (which is used by default). If set to 1, then min_freq and 
        max_freq must be equal (the filterbank will have 1 channel with central
        frequency equal to minFreq and maxFreq.
    min_freq : float, optional
        Minimum frequency (in Hz) used for ERB to determine central frequencies
        of gammatone channels. The paper suggests a value of 80 Hz.
    max_freq : float, optional
        Maximum frequency (in Hz) for ERB to determine central freqs of 
        gammatone channels. The paper suggests a value of 4000 Hz.
    Returns
    -------
    out : ndarray
        1D array of central frequencies to be used by the gammatone filters
        across all channels.
    """
    _ensure_pos_int(num_channels, "num_channels")
    assert min_freq > 0
    if num_channels == 1:
        assert min_freq == max_freq
    else:
        assert max_freq > min_freq

    out = np.empty((num_channels,),dtype=np.single)
    _centralFreqMapper(num_channels, min_freq, max_freq, out)
    return out

# implement sosGammatone
_argtypes = [np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                    flags='C_CONTIGUOUS'),
             np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                    flags=('C_CONTIGUOUS', 'WRITEABLE')),
             ctypes.c_float, ctypes.c_int, ctypes.c_int]
_sosGammatone = libmelex.sosGammatone
_sosGammatone.argtypes = _argtypes
_sosGammatoneFast = libmelex.sosGammatoneFast
_sosGammatoneFast.argtypes = _argtypes

def sosGammatone(audio_data, sample_rate, central_freq, out = None,
                 fast_mode = True):
    """
    Filters input data with the sosGammatone filter.

    The sosGammatone filter is an IIR filter that approximates a Gammatone
    filter

    Parameters
    ----------
    audio_data : array_like
        1D array of floating point audio data to be processed. It should hold 
        values nomalized between -1 and 1. Before being resampled, this will be
        cast to the np.single dtype
    sample_rate : int
        Sample rate of the audio data (in Hz)
    central_freq : float
        Frequency of the gammatone filter
    out : ndarray, None, optional
        A location where the result can be stored. If provided, this must have 
        the np.single dtype and have the same shape as audio_data.
    fast_mode : bool, optional
        Whether or not the fast implementation should be used. The slow 
        implementation, converts audio_data to np.double, doubles the sampling 
        rate performs the filtering, and then down samples and converts back to
        floats. Default is True. (The fast mode may not give quite as robust 
        results and could potentially run into aliasing issues)
    Returns
    -------
    y : ndarray
        The result of the filtering

    Notes
    -----
    See the documentation for sosGammatone in gammatoneFilter.h for a list of 
    potential problems that should be addressed, and alternative approximations 
    of the gammatone filter that could be considered.
    """

    _check_audio(audio_data)
    assert sample_rate > 0
    assert central_freq > 0

    if out is not None:
        assert isinstance(out,np.ndarray)
        assert out.shape == audio_data.shape
        assert out.dtype == audio_data.dtype
        y = out
    else:
        y = np.empty_like(audio_data)

    if fast_mode:
        func = _sosGammatoneFast
    else:
        raise ValueError("This does not currently work outside of fast mode")
        func = _sosGammatone

    func(audio_data, y, central_freq, sample_rate,
         audio_data.size)

    return y

_rollSigma = libmelex.rollSigma
_rollSigma.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float,
                       ctypes.c_int, ctypes.c_int, ctypes.c_int,
                       np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                              flags='C_CONTIGUOUS'),
                       np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                              flags=('C_CONTIGUOUS',
                                                     'WRITEABLE'))]
_rollSigma.restype = None

def roll_sigma(input_data, start_index, interval, sig_window_size,
               num_windows = None,
               scale_factor = (4./3.)**0.2):
    """
    Computes the rolling optimized sigma to be used by the kernel function.

    This function is adapted from the implementation of Rolling.std in the 
    pandas python package.

    Parameters
    ----------
    input_data : array_like
        1D array of floating point data to be processed of the np.single dtype.
    start_index : int
        The index  where the first window used to compute sigma is centered in 
        audio data.
    interval : int
        The number of elements in audio_data that the window advances over 
        between calculations of sigma.
    sig_window_size : int
        The width of the wind used to compute sigma. Must be a positive integer
        greater than 2
    num_windows : int, optional
        The number of values of sigma that should be computed. By default, this
        value is selcted so that the center of the last window is not located
        beyond the end of the array.
    scale_factor : float,optional
        The scale factor used to estimate the optimized standard deviation. By 
        default, this is the value for Silverman's rule of thumb: (4/3)^(1/5)

    Notes
    -----
    For values of sigma near the start and of buffer, the windows are
    automatically adjusted not to include values beyond the edge of the array.

    The current implementation allows the center of the window to extend past
    the end of the audio_data. However, for simplicity we require that 
    `start_index` alway lies with audio_data

    For a window centered at index `i`, the final element included in the 
    window is always located at `min(len(audio_data),i+sig_window_size//2)`
    If `sig_window_size` is even, then the first element included in the window 
    is at `max(0,i-sig_window_size//2 -1)`. Otherwise, the first element is 
    located at `max(0,i-sig_window_size//2)`.

    The current implementation casts the single precision input data to double 
    precision during the calculation and then casts back at the end
    """
    _check_1D_float_array(input_data)
    start_index = _ensure_int(start_index, "start_index", min_val = 0,
                              max_val = input_data.size - 1)

    
    sig_window_size = _ensure_int(sig_window_size, "sig_window_size",
                                  min_val = 2, max_val = None)
    interval = _ensure_pos_int(interval, "interval")

    # Compute the maximum center index of the final window.
    # First, compute size_left. The first index included in a window centered
    # at center_index is given by max(0,center_index-size_left).
    size_left = sig_window_size//2
    if size_left % 2 == 0:
        size_left -= 1
    # The final window must include at least 2 values
    max_window_center = input_data.size - 2 + size_left

    # now we can compute the maximum number of windows to include
    max_num_windows = (max_window_center-start_index)// interval 

    if num_windows is None:
        num_windows = max_num_windows
    else:
        num_windows = _ensure_int(num_windows, "num_windows",
                                  min_val = 1, max_val = max_num_windows)

    out = np.empty((num_windows,), np.single, order='C')

    _rollSigma(start_index, interval, scale_factor, sig_window_size,
               input_data.size, num_windows, input_data, out)
    return out

_pSMContribution = libmelex.pSMContribution
_pSMContribution.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int,
                             np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                                    flags='C_CONTIGUOUS'),
                             np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                                    flags='C_CONTIGUOUS'),
                             np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                                    flags=('C_CONTIGUOUS',
                                                           'WRITEABLE'))]
_pSMContribution.restypes = None

def psm_contribution(correntropy_win_size, interval, num_windows, input_data,
                     sigmas, psmatrix=None):
    """
    Iterates over each window in input_data and adds the pooled summary matrix
    contribution (the sum of of correntropy over all lags) of each window to 
    psmatrix. input from a given channel and computes Pooled Summary

    Parameters
    ----------
    correntropy_win_size : int
        The window size for the calculation of correntropy (specified as a 
        number of audio frames). The implementation assumes that the maximum 
        lag value is also given by this value.
    interval : int
        The hopsize, h, for computing the detection function, (in units of 
        audio frames). The paper suggests using 5 ms (sample_rate / 200); this 
        is used by default. Also acts as the interval between calculations of
        optimized sigma.
    num_windows : int
        The number of times that the pooled summary matrix contribution should 
        be computed from `input_buffer`. Typically, this is 1 larger what is 
        returned by `compute_detection_function_length`.
    input_data : array_like
        A 1D array of dtype np.single that holds the input (filtered) signal
        from which the pooled summary matrix calculation is performed. This 
        should have a length of at least 
        `(num_windows-1)*interval + 2*correntropy_win_size + 2`
    sigmas : array_like
        A 1D array of dtype np.single that holds the sigmas to be used to 
        compute the pooled summary matrix contribution at each interval. This 
        should have `num_windows` entries.
    psmatrix : array_like, optional
        If provided, the provided computed values are added to the appropriate
        elements held in this array. A provided array should be a 1D array of
        dtype np.single and a length of `num_windows`.
    Returns
    -------
    out : ndarray, optional
        This is only returned if psmatrix is not provided. This has a length of
        `num_windows`.

    Note
    ----
    The idea behind this function was to initialize `psmatrix` ahead of time as
    an array of zeros and then repeatedly call this function (but passing 
    different values to `input_data` and `sigmas` that correspond to the
    different channels).
    """
    num_windows = _ensure_pos_int(num_windows,"num_windows")
    correntropy_win_size = _ensure_pos_int(correntropy_win_size,
                                           "correntropy_win_size")
    interval = _ensure_pos_int(interval,"interval")

    _check_1D_float_array(input_data)
    min_input_len = max(num_windows-1,2)*interval + 2*correntropy_win_size + 2
    if input_data.size< min_input_len:
        msg = f"input_data must have a length of at least {min_input_len}"
        raise ValueError(msg)
    _check_1D_float_array(sigmas)
    assert sigmas.size == num_windows

    return_psmatrix = (psmatrix is None)
    
    if psmatrix is not None:
        _check_1D_float_array(psmatrix)
        assert psmatrix.size == num_windows
    else:
        psmatrix = np.zeros((num_windows,),dtype=np.single)

    _pSMContribution(correntropy_win_size, interval, num_windows,
                     input_data, sigmas, psmatrix)

    if return_psmatrix:
        return psmatrix

_computeDetFunctionLength = libmelex.computeDetFunctionLength
_computeDetFunctionLength.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]

def compute_detection_function_length(data_length, correntropy_win_size,
                                      interval):
    """
    Computes the length of the detection function that should be computed from
    audio data of a given size.
    
    Parameters
    ----------
    data_length : int
        The length of the input audio data.
    correntropy_win_size : int
        The window size for the calculation of correntropy (specified as a 
        number of audio frames). The implementation assumes that the maximum 
        lag value is also given by this value.
    interval : int
        The hopsize, h, for computing the detection function, (in units of 
        audio frames). The paper suggests using 5 ms (sample_rate / 200); this 
        is used by default. Also acts as the interval between calculations of
        optimized sigma.
    Returns
    -------
    det_func_length : int
        The length of the detection function that should be computed
    """
    data_length = _ensure_pos_int(data_length,"data_length")
    correntropy_win_size = _ensure_pos_int(correntropy_win_size,
                                           "correntropy_win_size")
    interval = _ensure_pos_int(interval,"interval")
    return _computeDetFunctionLength(data_length, correntropy_win_size,
                                     interval)



_simpleDetFunctionCalculation = libmelex.simpleDetFunctionCalculation
_simpleDetFunctionCalculation.argtypes \
    = [ctypes.c_int, ctypes.c_int,
       ctypes.c_float, ctypes.c_int,
       ctypes.c_int, ctypes.c_float, ctypes.c_float,
       ctypes.c_int, ctypes.c_int,
       np.ctypeslib.ndpointer(dtype=np.single, ndim=1, flags='C_CONTIGUOUS'),
       ctypes.c_int,
       np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                              flags=('C_CONTIGUOUS', 'WRITEABLE'))]

# need to update the following kwarg description
def compute_detection_function(audio_data, sample_rate,
                               num_channels = 64,
                               min_freq = 80., max_freq=4000.,
                               correntropy_win_size = None,
                               interval = None,
                               sig_window_size=None,
                               scale_factor = (4./3.)**0.2):
    """
    Calculates the detection function for use in the pairwise transient 
    detection method.

    Parameters
    ----------
    audio_data : array_like
        1D array of floating point audio data to be processed. It should hold 
        values nomalized between -1 and 1. Before being resampled, this will be
        cast to the np.single dtype
    sample_rate : int
        Sample rate of the audio data (in Hz)
    num_channels : int, optional
        Number of channels in gammatone filterbank. The suggested value by the 
        paper is 64 (which is used by default). If set to 1, then min_freq and 
        max_freq must be equal (the filterbank will have 1 channel with central
        frequency equal to minFreq and maxFreq.
    min_freq : float, optional
        Minimum frequency (in Hz) used for ERB to determine central frequencies
        of gammatone channels. The paper suggests a value of 80 Hz.
    max_freq : float, optional
        Maximum frequency (in Hz) for ERB to determine central freqs of 
        gammatone channels. The paper suggests a value of 4000 Hz.
    correntropy_win_size : int, optional
        Length of the correntropy window (in units of audio frames). The paper
	suggests a value of sampling rate / 80 (when min_freq = 80 Hz). This is 
        used by default.
    interval : int, optional
        The hopsize, h, for computing the detection function, (in units of 
        audio frames). The paper suggests using 5 ms (sample_rate / 200); this 
        is used by default. Also acts as the interval between calculations of
        optimized sigma.
    sig_window_size : int, optional
        The size of the window (in units of audio frames) used to compute the 
        optimized sigma for the detection function. The paper suggests a window
        that is 7 seconds wide. This value is used by default.
    scale_factor : float,optional
        The scale factor used to estimate the optimized standard deviation. By 
        default, this is the value for Silverman's rule of thumb: (4/3)^(1/5)

    Returns
    -------
    det_func : Sequence of floats
        Holds a sequence of floats that holds the detection function. The exact
        return type is subject to change.
    interval : int
        The size of the interval used in the calculation.
    """
    _check_audio(audio_data)
    
    assert num_channels > 0
    assert min_freq > 0
    if num_channels == 1:
        assert min_freq == max_freq
    else:
        assert max_freq > min_freq

    if correntropy_win_size is None:
        correntropy_win_size = int(sample_rate/80)
    assert correntropy_win_size > 0

    if interval is None:
        # set it to 5 ms
        interval = int(sample_rate/200)
    assert interval > 0

    if sig_window_size is None:
        # set it to about 7 seconds, and round up to the next largest odd
        # integer
        sig_window_size = 7*sample_rate
    assert sig_window_size > 0
    assert scale_factor > 0

    det_func_length = _computeDetFunctionLength(audio_data.size,
                                                correntropy_win_size, interval)
    
    det_func = np.empty((det_func_length,),dtype = np.single)

    result = _simpleDetFunctionCalculation(correntropy_win_size, interval,
                                           scale_factor, sig_window_size,
                                           num_channels, min_freq, max_freq,
                                           sample_rate, audio_data.size,
                                           audio_data, det_func.size,
                                           det_func)
    if result != 0:
        raise RuntimeError("Something went wrong")
    return det_func,interval

_detectTransients = libmelex.detectTransients
_detectTransients.argtypes = [np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                                     flags='C_CONTIGUOUS'),
                              ctypes.c_int,
                              ctypes.POINTER(intList_Struct)]

def detect_transients(detection_func):
    """
    Identifies transients from the values of the detection function

    Parameters
    ----------
    detection_func : Sequence of floats
        A sequence of floats that holds the detection function.

    Returns
    -------
    transients : IntList
        A sequence of integers corresponding to the interval of the detection 
        function where where transients are detected. The transients alternate 
        between being onsets and offsets. The first transient is always an 
        onset while the last transient is always on offset (there are always an
        even number of values).

    Note
    ----
    We added an extra step to this function (not detailed in the paper) in 
    which we normalize the detection function such that it has maximum 
    magnitude of 1.

    We added another extra step in which we dropped the last pair of detected 
    transients (the algorithm almost always return an extra false positive note 
    at the end)
    """
    detection_func = np.array(detection_func)
    max_capacity = len(detection_func)
    initial_capacity = min(20,max_capacity)
    transients = IntList(initial_capacity,max_capacity)
    print(detection_func)

    result = _detectTransients(detection_func, detection_func.size,
                               transients._struct_ptr)
    if result < 0:
        print(result)
        raise RuntimeError("Something went wrong while identifying transients")

    return transients


def pairwise_transient_detection(audio_data, sample_rate,
                                 det_func_kwargs = {}):
    """
    Pairwise detection of onsets and offsets

    Parameters
    ----------
    audio_data : array_like
        1D array of floating point audio data to be processed. It should hold 
        values nomalized between -1 and 1. Before being resampled, this will be
        cast to the np.single dtype
    sample_rate : int
        Sample rate of the audio data (in Hz)
    det_func_kwargs : dict,optional
        An optional set of key word arguments that get passed to 
        compute_detection_function.

    Returns
    -------
    transients : IntList
        A sequence of frames (indices of audio_data) where transients are 
        detected. The transients alternate between being onsets and offsets. 
        The first transient is always an onset while the last transient is 
        always on offset (there are always an even number of values)

    Notes
    -----
    Audio data from a wav file can be loaded with scipy.io.wavfile.read

    This does not wrap the pairwiseTransientDetection c function (but the 
    control flow is similar)
    """

    # compute the detection function
    det_func,interval = compute_detection_function(audio_data, sample_rate,
                                                   **det_func_kwargs)
    # identify the transients from det_func
    transients = detect_transients(det_func)

    # the values held in transients correspond to the interval of the detection
    # function where the transient was identified. We really want an estimate
    # of the audio frame where it occured (using the original sample rate)

    for i in range(len(transients)):
        # we may be able to be a little more clever the exact frames where the
        # transient is identified
        transients[i] *= interval

    return transients
    


if __name__ == '__main__':
    l = IntList()
    l.append(5)
    l.append(-100)

    print (len(l))
    for i in range(len(l)):
        print(l[i])
