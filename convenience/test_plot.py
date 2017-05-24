# Very Basic Python Script to plot original and weighted Fourier Spectra

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
from mpl_toolkits.axes_grid1 import make_axes_locatable


def load_from_file(fname):
    f = open(fname,'r')
    l = f.readline()
    if l[0] == '#':
        winSize = int(l.split('\t')[-1][:-1])
    l = f.readline()
    if l[0] == '#':
        sampleRate = int(l.split('\t')[-1][:-1])
    f.close()

    arr = np.loadtxt(fname).T

    return arr, winSize, sampleRate

def bin_to_freq(bin_i,fft_size,sample_rate):
    return bin_i *float(sample_rate)/fft_size

def note_names_frequencies():
    l = ["A ","A#","B ","C ","C#","D ","D#","E ","F ","F#","G ","G#",]
    tuning = 440
    notes = []
    freq = []
    for index,i in enumerate(range(1,106)):
        notes.append(l[index%12] + str((i+8)/12))
        freq.append(2.0**((i-49.)/12.) * tuning)
    return notes, freq

def note_name_axis(ax, specific_notes = None,min_freq = 0):
    """
    Adds a secondary axis of note names.
    """
    ax2 = ax.twinx()
    notes_temp,freqs_temp = note_names_frequencies()
    
    if specific_notes is not None:
        notes = []
        freqs = []
        for note,freq in zip(notes_temp,freqs_temp):
            if note[:2] in specific_notes and freq > min_freq:
                notes.append(note)
                freqs.append(freq)
            
    else:
        notes = notes_temp
        freqs = freqs_temp
    ax2.set_yticks(freqs)
    ax2.set_yticklabels(notes)
    ax2.set_ylim(ax.get_ylim())
    return ax2
    
def plot_spectra(ax,spectra, win_size,sample_rate,min_val = None):
    ymax,xmax = spectra.shape
    if min_val is not None:
        spectra = np.copy(spectra)
        spectra[spectra<min_val] = min_val
    out = ax.imshow(spectra,origin='lower',aspect = 'auto',
                    extent=[0,xmax,0,bin_to_freq(ymax,win_size,sample_rate)],
                    norm = LogNorm())
    return out

if __name__ == '__main__':
    arr, win_size, sample_rate = load_from_file('t_original.txt')
    #arr, win_size, sample_rate = load_from_file('t_weighted.txt')
    
    
    fig = plt.figure()
    ax = fig.add_subplot(111)

    im = plot_spectra(ax,arr, win_size,sample_rate,1.e-7)
    divider = make_axes_locatable(ax)
    ax.set_ylabel("Frequency (Hz)")
    
    

    ax.set_ylim((0,2500))
    #note_name_axis(ax, specific_notes = ["C "],min_freq = 200)
    cax = divider.append_axes("right", size="5%", pad=0.05)
    plt.colorbar(im, cax=cax,label = 'Weighting')

    plt.show()
