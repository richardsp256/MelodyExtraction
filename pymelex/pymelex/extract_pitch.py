import ctypes
import os.path

import numpy as np

from .shared_lib import libmelex
from .misc import _check_audio, _ensure_int, _ensure_pos_int

# load in the different pitch strategies. This is not strictly necessary
# alternatively we could just call choosePitchStrategy
_argtypes = libmelex.get_CFUNCTYPE_argtypes('PitchStratFunc_t')

_HPSDetectionStrategy = libmelex.HPSDetectionStrategy
_HPSDetectionStrategy.argtypes = _argtypes
_BaNaDetectionStrategy = libmelex.BaNaDetectionStrategy
_BaNaDetectionStrategy.argtypes = _argtypes
_BaNaMusicDetectionStrategy = libmelex.BaNaMusicDetectionStrategy
_BaNaMusicDetectionStrategy.argtypes = _argtypes

_choosePitchStrategy = libmelex.choosePitchStrategy
_choosePitchStrategy.argtypes = [ctypes.c_char_p]
_choosePitchStrategy.restype = ctypes.POINTER(libmelex.PitchStratFunc_t)

strats = {}

class audioInfo_Struct(ctypes.Structure):
    """
    Represents the audioInfo Struct
    """
    _fields_ = [("frames",           ctypes.c_int64),
                ("samplerate",       ctypes.c_int)]

_ExtractPitch = libmelex.ExtractPitch
_ExtractPitch.argtypes = [np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
				                 flags=('C_CONTIGUOUS')),
                          np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
				                 flags=('C_CONTIGUOUS',
                                                        'WRITEABLE')),
                          audioInfo_Struct,
                          ctypes.c_int, ctypes.c_int, ctypes.c_int,
                          ctypes.POINTER(libmelex.PitchStratFunc_t),
                          ctypes.c_int, ctypes.c_int, ctypes.c_char_p]

_NumSTFTBlocks = libmelex.NumSTFTBlocks
_NumSTFTBlocks.argtypes = [audioInfo_Struct, ctypes.c_int, ctypes.c_int]

def extract_pitch(audioData, sample_rate, unpadded_window_size,
                  full_window_size, window_interval, pitch_strat, hpsOvr = 2,
                  verbose = 0, prefix = None):
    """
    Extracts Pitches from an audio stream

    This involves taking a short-time fourier transform (STFT) and passing the 
    result to a callback function.

    Parameters
    ----------
    audio_data : array_like
        1D array of floating point audio data to be processed. It should hold 
        values nomalized between -1 and 1. Before being resampled, this will be
        cast to the np.single dtype
    sample_rate : int
        Sample rate of the audio data (in Hz)
    unpadded_window_size : int
        The number of elements from audio_data included in each window used for
        the DFT. This should be positive.
    full_window_size : int
        The size of the window used for each DFT. This must be at least as 
        large as window_interval. When it's larger, the window is first filled 
        with entries from audio_data and then the remaining space is padded with
        zeros. (Zero-padding is used to control the size of the frequency bins 
        in the resulting spectrum).
    window_interval : int
        The interval between the first entry in of audio_data between 
        consecutive windows.
    pitchStrategy : string or callable
        Either the name of a built-in strategy (e.g. 'HPS', 'BaNa', 
        'BaNaMusic') or a callable
    hpsOvr : int
        A value that get's passed to the pitchStrategy. Of the built-in pitch
        strategies, this only has an effect on 'HPS'; it sets the number of
        harmonic product specturm overtones.
    verbose : bool
        Whether to print debugging information
    prefix : None or string
        This is either None or a string that get's determines 2 paths where 
        spectrogram data is saved. The spectrogram produced directly by the 
        STFT is saved at '{prefix}_original.txt'. An other spectrogram that may
        or may not be different (it depends on the callback function) is saved
        at '{prefix}_weighted.txt' after using the callback function.
    Returns
    -------
    out_array : `np.ndarray`
        A 1D array of the identified frequencies.
    """

    # argument checking
    _check_audio(audioData)
    sample_rate = _ensure_pos_int(sample_rate, 'sample_rate')
    unpadded_window_size = _ensure_pos_int(unpadded_window_size,
                                           'unpadded_window_size')
    full_window_size = _ensure_int(full_window_size, 'full_window_size',
                                   min_val = unpadded_window_size)
    window_interval = _ensure_pos_int(window_interval, 'window_interval')

    # retrieve the pitch_strat callback
    if isinstance(pitch_strat,str):
        callback_ptr = _choosePitchStrategy(pitch_strat.encode('utf-8'))
        if not bool(callback_ptr):
            raise ValueError("'" + pitch_strat +
                             "' is not the name of a known strategy")
    elif callable(pitch_strat):
        callback_ptr = libmelex.pitchStratFunc_t(pitch_strat)
    else:
        raise ValueError("pitch_strat must either be callable or the name of "
                         "a known strategy")

    hpsOvr = _ensure_pos_int(hpsOvr, "hpsOvr")
    verbose = _ensure_int(verbose, "verbose",0,1)

    # ensure prefix is either None OR that it involves a real path
    assert prefix is None or os.path.isdir(os.path.dirname(prefix))

    # initialize an audioInfo struct
    audio_info = audioInfo_Struct()
    audio_info.frames = audioData.size
    audio_info.samplerate = sample_rate

    # get the length of the output
    out_length = _NumSTFTBlocks(audio_info, unpadded_window_size,
                                window_interval)
    assert out_length > 0

    # allocate output buffer
    out_array = np.empty((out_length,),dtype = np.single)

    result = _ExtractPitch(audioData, out_array, audio_info,
		           unpadded_window_size, full_window_size,
                           window_interval, callback_ptr, hpsOvr, verbose,
                           prefix)

    if result <= 0:
        raise RuntimeError("Something went wrong")
    elif result != out_length:
        raise ValueError("Unexpected output length")

    return out_array
