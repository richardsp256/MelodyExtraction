
# Definitions of some basic frequently used utility functions (e.g. for
# checking arguments).


import numpy as np

def _check_1D_float_array(input_data, name = "input_data"):
    if not input_data.flags['C_CONTIGUOUS']:
        raise ValueError(name + " must be C contiguous")
    if input_data.dtype != np.single:
        raise ValueError(name + " must have the np.single dtype")
    if input_data.ndim != 1:
        raise ValueError(name + " must be 1 dimensional")

def _check_audio(input_audio, name = "input_audio"):
    """
    Checks the properties of a numpy array meant to hold input audio
    """
    _check_1D_float_array(input_audio, name = "input_data")
    if np.abs(input_audio).max() > 1:
        # this will also pick up nans and infs
        raise ValueError(name + " contains an entry with a magnitude "
                         + "exceeding 1.")

def _ensure_int(val, arg_name, min_val = None, max_val = None):
    # checks that val is an int. If it is not an int and it is possible to
    # coerce, then it's coerced

    if isinstance(val,int):
        coerced = val
    else:
        coerced = int(val)
    if coerced != val:
        raise TypeError("{} must be an int".format(arg_name))

    if (min_val is not None) and val < min_val:
        raise ValueError("{} must be at least {}".format(arg_name, min_val))
    if (max_val is not None) and val > max_val:
        msg = "{} must be no larger than {}".format(arg_name, max_val)
        raise ValueError(msg)

    return val

def _ensure_pos_int(val,arg_name):
    return _ensure_int(val, arg_name, 1)
