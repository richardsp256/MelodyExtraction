import collections.abc
import ctypes
from math import ceil

import numpy as np

from .shared_lib import libmelex
from .resample import _check_audio


class intList_Struct(ctypes.Structure):
    """
    Represents the intList struct
    """
    _fields_ = [("array",        ctypes.POINTER(ctypes.c_int)),
                ("length",       ctypes.c_int),
                ("capacity",     ctypes.c_int),
                ("max_capacity", ctypes.c_int)]

intListCreate = libmelex.intListCreate
intListCreate.argtypes = [ctypes.c_int, ctypes.c_int]
intListCreate.restype = ctypes.POINTER(intList_Struct)

intListDestroy = libmelex.intListDestroy
intListDestroy.argtypes = [ctypes.POINTER(intList_Struct)]

intListGet = libmelex.intListGet
intListGet.argtypes = [ctypes.POINTER(intList_Struct), ctypes.c_int]
intListGet.restype = ctypes.c_int

intListAppend = libmelex.intListAppend
intListAppend.argtypes = [ctypes.POINTER(intList_Struct), ctypes.c_int]
intListAppend.restype = ctypes.c_int


class IntList(collections.abc.MutableSequence):
    """
    Python wrapper class of intList

    Parameters
    ----------
    arg : int, collections.abc.Collection containing ints, optional
        If this is an int, this sets the initial capacity. If it's a sequence
        then it initializes the class with the elements of the sequence.
    max_capactiy : int, optional
        Specifies a maximum capacity
    """
    
    def __init__(self, arg = None, max_capacity = None):

        initial_vals = []
        if arg is None:
            capacity = 10
        elif isinstance(arg, int):
            capacity = arg
        elif isinstance(arg, collections.abc.Collection):
            capacity = len(arg)
            initial_vals = arg
        else:
            raise ValueError()

        if isinstance(max_capacity, int):
            if max_capacity<=0:
                raise ValueError("max capacity must be greater than 0")
        elif max_capacity is None:
            max_capacity = 0 # this is recognized as the default value
        else:
            msg = "max_capacity must be an int or None, not {}"
            raise TypeError(msg.format(max_capcity.__class__.__name__))

        self._struct_ptr = intListCreate(capacity,max_capacity)

        for elem in initial_vals:
            self._check_val_type(val)
            self.append(initial_vals)

    def __del__(self):
        intListDestroy(self._struct_ptr)

    def _check_index(self, index):
        if isinstance(index, int):
            if index < 0:
                raise IndexError("index must not be negative")
            elif index >= len(self):
                raise IndexError("index must be less than length")
        elif isinstance(index, (slice, tuple)):
            raise ValueError("Can't handle slices")
        else:
            ind_t = index.__class__.__name__
            raise TypeError("indices must be integers, not {}".format(ind_t))

    def _check_val_type(self,val):
        # may want to check the size of the int
        assert isinstance(val,int)

    def __len__(self):
        return self._struct_ptr.contents.length

    def __getitem__(self,index):
        self._check_index(index)
        return intListGet(self._struct_ptr, index)

    def __setitem__(self, index, val):
        self._check_index(index)
        self._check_val_type(val)
        return self._struct_ptr.contents.array[index]

    def append(self, val):
        self._check_val_type(val)
        temp = intListAppend(self._struct_ptr,val)
        if temp == 0:
            raise MemoryError()

    # Need these methods to make this into a Mutable Sequence
    def __delitem__(self,index):
        self._check_index(index)
        length = len(self)
        for i in range(index,length-1):
            self[i] = self[i+1]
        self._struct_ptr.contents.length = length - 1

    def insert(self, index, val):
        self._check_val_type(val)
        self._check_index(index)
        length = len(self)
        self.append(self[length-1])

        for i in range(length-1,index,-1):
            self[i] = self[i-1]
        self[index] = val


_computeDetFunctionLength = libmelex.computeDetFunctionLength
_computeDetFunctionLength.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]

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
    if result != 1:
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
