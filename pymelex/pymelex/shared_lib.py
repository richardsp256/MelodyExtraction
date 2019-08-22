

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
    """

    instance = None

    def __init__(self,path):
        if self.instance is None:
            self.instance = ctypes.CDLL(path)
    def __getattr__(self,name):
        """
        Forwards all attribute access onto the wrapped instance
        """
        return getattr(self.instance,name)


# relative to this file, the path to the shared object file is at:
_rel_so_path = "../../src/libmelodyextraction.so"

# get the current file directory
_cur_file_dir = os.path.dirname(os.path.abspath(__file__))

# find the absolute path to the shared object file
_so_path = os.path.abspath(os.path.join(_cur_file_dir,_rel_so_path))

if not os.path.isfile(_so_path):
    raise RuntimeError("Cannot find a file at {}".format(_so_path))

libmelex = LibMelex(_so_path)
