

import ctypes
import os.path

class LibMelex:
    """
    A singleton object to house the shared library. 

    This object just wraps an instance of `ctypes.CDLL` (Unclear how necessary
    this actually is).

    Inspired by: 
        https://python-3-patterns-idioms-test.readthedocs.io/en/
        latest/Singleton.html

    Could be a little more careful to ensure I don't run into name collisions
    """

    instance = None

    # items in cfunc_types are 4-tuples (restype, argtypes, init_kwargs, obj)
    cfunc_types = {}

    def __init__(self,path):
        if self.instance is None:
            self.instance = ctypes.CDLL(path)

    def register_CFUNCTYPE(self, name, restype, *argtypes, **kwargs):
        """
        Builds and register a function proto-type

        After registering it, the proto-type can be accessed by the attribute 
        of the object.
        """
        # after registering the CFUNCTYPE, we can access it as a attribute of
        # the object
        out = ctypes.CFUNCTYPE(restype, *argtypes, **kwargs)
        self.cfunc_types[name] = (restype, argtypes, kwargs, out)
        return out

    def get_CFUNCTYPE_restype(self, name):
        """
        Return the resolution type of a registered CFUNCTYPE
        """
        return self.cfunc_types[name][0]

    def get_CFUNCTYPE_argtypes(self, name):
        """
        Return the resolution type of a registered CFUNCTYPE
        """
        return self.cfunc_types[name][1]

    def get_registered_CFUNCTYPEs(self, name):
        return self.cfunc_types.keys()

    def __getattr__(self,name):
        """
        Forwards all attribute access onto the wrapped instance
        """

        try:
            return getattr(self.instance,name)
        except AttributeError:
            if name in self.cfunc_types:
                return self.cfunc_types[name][-1]
            else:
                return object.__getattribute__(self,name)


# relative to this file, the path to the shared object file is at:
_rel_so_path = "../../.libs/libmelodyextraction.so"

# get the current file directory
_cur_file_dir = os.path.dirname(os.path.abspath(__file__))

# find the absolute path to the shared object file
_so_path = os.path.abspath(os.path.join(_cur_file_dir,_rel_so_path))

if not os.path.isfile(_so_path):
    raise RuntimeError("Cannot find a file at {}".format(_so_path))

libmelex = LibMelex(_so_path)

import numpy as np

# register PitchStrategyFunc prototype
_argtypes = [np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                    flags=('C_CONTIGUOUS','WRITEABLE')),
             ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
             ctypes.c_int,
             np.ctypeslib.ndpointer(dtype=np.single, ndim=1,
                                    flags=('C_CONTIGUOUS','WRITEABLE'))]
libmelex.register_CFUNCTYPE("PitchStratFunc_t", ctypes.c_int,*_argtypes)
