import ctypes

import numpy as np

from .shared_lib import libmelex
from .FixedTypeList import distinctList_Struct
from .NestedDistinctList import NestedDistinctList

__all__ = ['select_candidates', 'find_BaNa_candidates']

_distinctList_dblptr = ctypes.POINTER(ctypes.POINTER(distinctList_Struct))

_candidateSelection = libmelex.candidateSelection
_candidateSelection.argtypes = [
    _distinctList_dblptr, ctypes.c_long,
    np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                           flags=('C_CONTIGUOUS', 'WRITEABLE'))
]
_candidateSelection.restype = None

# it would probably be nice to specify subsets of nested_window_candidate_list
def select_candidates(nested_window_candidate_list):
    """
    Selects the best candidates for the fundamental frequency for each window

    Parameters
    ----------
    nested_window_candidate_list : NestedDistinctList
        Holds the list of candidates at each window
    Returns
    -------
    fundamentals : np.ndarray
        Array holding the best fundamental frequencies for each window. The
        length is the same as len(nested_window_candidate_list)
    """

    assert isinstance(nested_window_candidate_list, NestedDistinctList)
    length = len(nested_window_candidate_list)
    fundamentals = np.empty((length,),dtype = np.single)
    _candidateSelection(nested_window_candidate_list._get_arr_pointer(),
			length, fundamentals)
    return fundamentals

def _BaNaFindCandidates_errcheck(result,func,args):
    # should come back to look at this and try to decode the error
    if result != 0:
        raise RuntimeError(
            "There was some kind of error in _BaNaFindCandidates: {}".\
            format(result)
            )

_BaNaFindCandidates = libmelex.BaNaFindCandidates
_BaNaFindCandidates.argtypes = [
    np.ctypeslib.ndpointer(dtype=np.single, ndim=1, flags = 'C_CONTIGUOUS'),
    ctypes.c_int, ctypes.c_int, ctypes.c_int,
    ctypes.c_float, ctypes.c_float, ctypes.c_bool, ctypes.c_float,
    np.ctypeslib.ndpointer(dtype=np.single, ndim=1, flags = 'C_CONTIGUOUS'),
    ctypes.c_float, _distinctList_dblptr
]
_BaNaFindCandidates.restype = ctypes.c_int
_BaNaFindCandidates.errcheck = _BaNaFindCandidates_errcheck

def find_BaNa_candidates(spectrogram, p, f0min, f0max, first, xi,
                         frequencies, smooth_width):
    """
    Identifies fundamental frequency candidates with the BaNa pitch detection 
    algorithm.

    Parameters
    ----------
    spectrogram: np.ndarray
        2D array holding a series of fourier spectra (this should give the 
        magnitudes of each entry). This should be C_contiguous. Time should 
        increase along axis 0 and frequency should vary along axis 0.
    p: int
        The number of harmonic peaks to search for. This must be at least 1.
    f0min: float
        The minimum fundamental frequency to search for
    f0max: float
        The maximum fundamental frequency to search for
    first: bool
        If true, the only the lowest frequency peaks are consider. If false, 
        then all of the peaks are considered
    xi: float
        The tolerance for looking for harmonic peaks
    frequencies: np.ndarray
        Array of center frequencies in each bin.
    smooth_width: float
        The number of frequency bins to include in each pass of the tophat 
        smoothing. It's recommended that this is 50 Hz.

    Returns
    -------
    window_candidates: NestedDistinctList
        Holds the list of candidates for each window.
    """
    assert isinstance(spectrogram, np.ndarray)
    assert spectrogram.ndim == 2
    num_windows, num_freq_bins = spectrogram.shape

    assert isinstance(frequencies, np.ndarray)
    if frequencies.size != num_freq_bins:
        raise ValueError(
            ("Since spectrogram.shape = ({},{}), frequencies.size should be "
             "{}, not {}").format(num_windows, num_freq_bins, num_freq_bins,
                                  frequencies.size)
        )
    elif frequencies.min() < 0.:
        raise ValueError("The minimum frequency shouldn't be negative")

    assert isinstance(p,int) and p > 0
    assert f0min > 0.
    assert f0max > f0min
    assert isinstance(first,bool)
    assert smooth_width > 0.

    assert xi > 0.

    windowCandidatesArrType = ctypes.POINTER(distinctList_Struct) * num_windows
    windowCandidatesArr = windowCandidatesArrType()

    _BaNaFindCandidates(spectrogram.flatten(), spectrogram.size, num_freq_bins,
                        p, f0min, f0max, first, xi,
			frequencies.astype(np.single), smooth_width,
                        ctypes.byref(windowCandidatesArr[0]))
    return NestedDistinctList(windowCandidatesArr, num_windows)
