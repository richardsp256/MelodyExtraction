import numpy as np


def save_float32(fname,array):
    temp = array.astype('<f4',order='C',casting='equiv',copy=True)
    temp.tofile(fname)

def save_double64(fname,array):
    temp = array.astype('<f8',order='C',casting='equiv',copy=True)
    temp.tofile(fname)

def save_int32(fname,array):
    temp = array.astype('<i4',order='C',casting='equiv',copy=True)
    temp.tofile(fname)

def save_int64(fname,array):
    temp = array.astype('<i8',order='C',casting='equiv',copy=True)
    temp.tofile(fname)

if __name__ == '__main__':
    a = np.arange(10)
    print a.dtype
    b = a.astype('<i4')
    print b
    print b.dtype.byteorder
