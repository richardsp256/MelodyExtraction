This tool is designed to analyze an audio file and output a MIDI 
representation of the melodic line.

This is a work-in-progress, with not all components implemented yet.

This project utilizes the FFTW, Libsndfile, libsamplerate, and libfvad
libraries. It was tested using fftw-3.3.4, libsndfile-1.0.26, and
libsamplerate-0.1.9

INSTALLATION
------------
Download and extract FFTW, Libsndfile, libsamplerate, and libfvad to the
project folder.

From the FFTW folder run the commands:

	./configure --enable-float
	make

From the Libsndfile folder run the commands:

	./configure
	make

To install libfvad, autoconf and automake must be installed. From the libfvad
folder run the commands:

	./bootstrap
	./configure
	make

From the libsamplerate folder run the commands:

	./configure
	make

Now that all libraries are compiled, you can compile and run the project.

If for any reason you wish to recompile these libraries, you must first remove
the current configuration files with
	
	make distclean

in that library's directory.