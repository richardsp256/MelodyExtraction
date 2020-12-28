from abc import abstractmethod
import collections.abc
import ctypes

import numpy as np

from .shared_lib import libmelex

# this could probably be consolidated with IntList

class intList_Struct(ctypes.Structure):
    """
    Represents the intList struct
    """
    _fields_ = [("array",        ctypes.POINTER(ctypes.c_int)),
                ("length",       ctypes.c_int),
                ("capacity",     ctypes.c_int)]


class DistinctCandidate(ctypes.Structure):
    """
    Represents the intList struct
    """
    _fields_ = [("frequency",       ctypes.c_float),
                ("confidence",      ctypes.c_int),
                ("cost",            ctypes.c_float),
                ("indexLowestCost", ctypes.c_int)]

class distinctList_Struct(ctypes.Structure):
    """
    Represents the intList struct
    """
    _fields_ = [("array",        ctypes.POINTER(DistinctCandidate)),
                ("length",       ctypes.c_int),
                ("capacity",     ctypes.c_int)]


_LIST_PROP = (
    ('intList', ctypes.c_int, intList_Struct),
    ('distinctList', DistinctCandidate, distinctList_Struct)
)

def _append_errcheck(result, func, args):
    if result != 0:
        raise MemoryError()

_LIST_PROP_DICT = {}
for (prefix, element_type, struct_type) in _LIST_PROP:
    cur = {}

    cur['struct_class'] = struct_type
    cur['struct_ptr_class'] = ctypes.POINTER(struct_type)

    cur['create'] = getattr(libmelex, prefix + 'Create')
    cur['create'].argtypes = [ctypes.c_int]
    cur['create'].restype = ctypes.POINTER(struct_type)

    cur['destroy'] = getattr(libmelex, prefix + 'Destroy')
    cur['destroy'].argtypes = [ctypes.POINTER(struct_type)]

    cur['get'] = getattr(libmelex, prefix + 'Get')
    cur['get'].argtypes = [ctypes.POINTER(struct_type), ctypes.c_int]
    cur['get'].restype = element_type

    cur['append'] = getattr(libmelex, prefix + 'Append')
    cur['append'].argtypes = [ctypes.POINTER(struct_type), element_type]
    cur['append'].restype = ctypes.c_int
    cur['append'].errcheck = _append_errcheck

    _LIST_PROP_DICT[prefix] = cur

def _check_index(index, length):
    if isinstance(index, int):
        if index < 0:
            raise IndexError("index must not be negative")
        elif index >= length:
            raise IndexError("index must be less than length")
        elif isinstance(index, (slice, tuple)):
            raise ValueError("Can't handle slices")
    else:
        ind_t = index.__class__.__name__
        raise TypeError("indices must be integers, not {}".format(ind_t))

class _FixedTypeList(collections.abc.MutableSequence):
    """
    Python wrapper class of a fixed-type list

    Parameters
    ----------
    arg : int, collections.abc.Collection, optional
        If this is an int, this sets the initial capacity. If it's a sequence
        then it initializes the class with the elements of the sequence.

    Notes
    -----
    Additionally, arg can be a 2-tuple holding a pointer to an existing array
    and the parent object that manages the lifetime of that pointer. This is 
    not meant for public consumption.
    """
    @abstractmethod
    def _get_prop_dict(self):
        pass

    @abstractmethod
    def _check_val_type(self,val):
        pass

    def __init__(self, arg = None):
        # the _ref attribute is used when the class wraps a pointer whose
        # lifetime is managed by a separate object. Basically, the _ref
        # attribute is set equal to the parent object to make sure that the
        # underlying object is not deallocated while this object is alive
        self._ref = None

        if ( isinstance(arg, tuple) and len(arg) == 2 and
             isinstance(arg[0],self._get_prop_dict()['struct_ptr_class']) ):
            self._struct_ptr, self._ref = arg
            assert self._ref is not None
        else:
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

            #if isinstance(max_capacity, int):
            #    if max_capacity<=0:
            #        raise ValueError("max capacity must be greater than 0")
            #elif max_capacity is None:
            #    max_capacity = 0 # this is recognized as the default value
            #else:
            #    msg = "max_capacity must be an int or None, not {}"
            #    raise TypeError(msg.format(max_capcity.__class__.__name__))

            func = self._get_prop_dict()['create']
            self._struct_ptr = func(capacity)

            for elem in initial_vals:
                self._check_val_type(val)
                self.append(initial_vals)

    def __del__(self):
        # when self._ref is not None, the lifetime of the pointer is managed by
        # the parent object
        if self._ref is None:
            destroy = self._get_prop_dict()['destroy']
            destroy(self._struct_ptr)

    def _check_index(self, index):
        _check_index(index,len(self))

    def __len__(self):
        return self._struct_ptr.contents.length

    def __getitem__(self,index):
        self._check_index(index)
        getter = self._get_prop_dict()['get']
        return getter(self._struct_ptr, index)

    def __setitem__(self, index, val):
        self._check_index(index)
        self._check_val_type(val)
        return self._struct_ptr.contents.array[index]

    def append(self, val):
        self._check_val_type(val)
        append = self._get_prop_dict()['append']
        temp = append(self._struct_ptr,val)

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

class IntList(_FixedTypeList):
    def _get_prop_dict(self):
        return _LIST_PROP_DICT['intList']

    def _check_val_type(self,val):
        # may want to check the size of the int
        assert isinstance(val,int)

class DistinctCandidateList(_FixedTypeList):
    def _get_prop_dict(self):
        return _LIST_PROP_DICT['distinctList']

    def _check_val_type(self,val):
        # may want to check the size of the int
        assert isinstance(val,DistinctCandidate)
