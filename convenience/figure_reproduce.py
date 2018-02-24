from scipy.io import wavfile 
import numpy as np

import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm

def load_wave(fname):
    fs, array = wavfile.read(fname)
    return fs,array

def load_from_file(fname):
    f = open(fname,'r')
    l = f.readline()
    if l[0] == '#':
	pass
    l = f.readline()
    if l[0] == '#':
        interval = int(l.split('= ')[-1][:-1])
    l = f.readline()
    if l[0] == '#':
        sampleRate = int(l.split('= ')[-1][:-1])
    f.close()
    print interval,sampleRate
    temp = np.loadtxt(fname)
    return temp.reshape(temp.shape[::-1]),interval,sampleRate

def calcDetFunction(time, channel_data):
    pooled_summary_matrix = np.sum(channel_data, axis=0)
    det_function = np.diff(pooled_summary_matrix)
    t = time[:-1]
    return t,det_function

def plot_channel_data(ax,time, channel_data,cmap='viridis'):
    temp = channel_data.shape[0]
    extent = [np.amin(time),np.amax(time),1,temp]
    #extent = [1,temp,np.amin(time),np.amax(time)]
    print channel_data.shape
    ax.imshow(channel_data, origin='lower',extent=extent, cmap=cmap,
	      interpolation='none',aspect='auto',norm=LogNorm())

if __name__ == '__main__':
    fs,array = load_wave('/home/matthew/Projects/metf/audio/'
			 'super_Mario_bros.wav')
    temp = array.astype
    mapped = array.astype(np.float64)/float(np.amax(np.abs(array)))
    t = np.arange(np.alen(mapped)).astype(np.float)
    time1 = t/float(fs)

    temp = load_from_file('../src/savedChannelPSMContrib.txt')
    data,interval,samplerate = temp

    
    delta_t = float(interval)/samplerate
    time_channel = np.arange(data.shape[1])*delta_t
    time_df, det_function = calcDetFunction(time_channel, data)


    
    fig, ax_array = plt.subplots(nrows = 3, ncols = 1, sharex = True)
    ax_array[0].plot(time1,mapped,'b-')
    ax_array[0].set_ylim((-1.2,1.2))

    plot_channel_data(ax_array[1],time_channel, data,cmap='viridis_r')
    ax_array[1].set_ylim((0,65))

    ax_array[2].plot(time_df, det_function)
    ax_array[2].set_xlabel('$t$ (seconds)')
    ax_array[2].set_ylabel("$\Delta W$")
    plt.show()
    
