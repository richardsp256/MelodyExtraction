
import collections.abc
import ctypes

import numpy as np

from .shared_lib import libmelex

class intList_Struct(ctypes.Structure):
    """
    Represents the intList struct
    """
    _fields_ = [("array",        ctypes.POINTER(ctypes.c_int)),
                ("length",       ctypes.c_int),
                ("capacity",     ctypes.c_int)]

intListCreate = libmelex.intListCreate
intListCreate.argtypes = [ctypes.c_int]
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
    """
    
    def __init__(self, arg = None):

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

        self._struct_ptr = intListCreate(capacity)

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
