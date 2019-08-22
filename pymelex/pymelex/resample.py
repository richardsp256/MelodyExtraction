import ctypes

import numpy as np

from .shared_lib import libmelex

def _check_audio(input_audio):
    """
    Checks the properties of a numpy array meant to hold input audio
    """
    if not input_audio.flags['C_CONTIGUOUS']:
        raise ValueError("input_audio must be C contiguous")
    if input_audio.dtype != np.single:
        raise ValueError("input_audio must have the np.single dtype")
    if input_audio.ndim != 1:
        raise ValueError("input_audio must be 1 dimensional")
    if np.abs(input_audio).max() > 1:
        # this will also pick up nans and infs
        raise ValueError("input_audio contains an entry with a magnitude "
                         "exceeding 1.")

_ResampledLength = libmelex.ResampledLength
_ResampledLength.argtypes = [ctypes.c_int, ctypes.c_float]

_Resample = libmelex.Resample
# args are float* input, int len, float sampleRatio, float* output
_Resample.argtypes = [np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                             flags='C_CONTIGUOUS'),
                      ctypes.c_int, ctypes.c_float,
                      np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                             flags=('C_CONTIGUOUS',
                                                    'WRITEABLE'))]

def resample(input_audio, sample_ratio):
    """
    Resamples audio.

    Parameters
    ----------
    input_audio : array_like
        1D array of floating point audio data to be resampled. It should hold 
        values nomalized between -1 and 1. Before being resampled, this will be
        cast to the np.single dtype.
    sample_ratio : float
        Ratio of the new sample rate to the old sample rate (e.g. a value of 2.
        doubles the sample rate).
    Returns
    -------
    resampled : `numpy.ndarray`
        1D array of resampled audio.
    """

    data = np.ascontiguousarray(input_audio,np.single)
    _check_audio(input_audio)
    if sample_ratio <= 0:
        raise ValueError("sample_ratio must be a positive value")

    # this i
    guessed_out_length = _ResampledLength(data.shape[0],sample_ratio)
    if guessed_out_length <= 0:
        raise ValueError("The resampled length exceeds the limits of "
                         "int data type")
    
    output = np.empty((guessed_out_length),np.single,order='C')

    out_length = _Resample(data,data.shape[0],sample_ratio,output)

    return output[:out_length]
