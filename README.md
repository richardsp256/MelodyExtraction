This tool is designed to take in an audio file as input and output a MIDI 
representation of the melodic line.

This is a work-in-progress, with not all components implemented yet.

This project utilizes the FFTW, Libsndfile, libsamplerate, and libfvad
libraries. It was tested using fftw-3.3.4, libsndfile-1.0.26, and
libsamplerate-0.1.9.

This project utilizes Check v0.12.0 for unit testing. Presently, Check
0.12.0 must be installed to build this project (this needs to be fixed
in the future).

INSTALLATION
------------
Download and extract FFTW, Libsndfile, libsamplerate, and libfvad to the
project folder.

From the FFTW folder run the commands:

	./configure --enable-float CFLAGS=-fPIC
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




CMake 3.x is used to build the project. Earlier versions of CMake are untested. Build the executable and archived library with 
	
	cmake .
	make

in the project directory.

Unit Testing
------------

Run tests on the built project with
	
	make test

in the project directory. If any of the tests fail, specifics can be found by
calling the executables for the tests that fail in the tests subdirectory.

Note: If using the project on Ubuntu, do NOT install Check from the default
repository. An older version of Check will be installed and errors will occur
while building the unit tests (last checked on 12/27/17). Instead, install
Check from the official website.

Shared Library Usage
--------------------
To make use of the functionallity in this project, include melodyextraction.h
and tell the compiler to use libmelodyextraction.a
