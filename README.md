This tool is designed to take in an audio file as input and output a MIDI 
representation of the melodic line.

This is a work-in-progress, with not all components implemented yet.

This project utilizes the FFTW, Libsndfile, libsamplerate, and libfvad
libraries. It was tested using fftw-3.3.4, libsndfile-1.0.26, and
libsamplerate-0.1.9.

This project utilizes Check v0.12.0 for unit testing. Presently, Check
0.12.0 is required to build the project, even if tests are not being
run (This needs to be addressed in future versions). 

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


If you built/installed any of the preceding libraries at locations other than
the project folder, you will need to follow the instructions in the CMake file
to specify the paths to where the libraries can be found.


CMake 3.x is used to build the project. Earlier versions of CMake are untested. From the repository's root directory, build the executable and archived library with
	
	mkdir build # could be named something else
	cd build
	cmake ..
	make

This will build the `extract` executable (and the libraries) in `build/src`.
Calling

	make install

from within the `build` directory will copy `extract` into the repository's root directory and will copy the libraries into `.libs`, which is also placed in the root directory (handling of the libraries may change going forward).

By default, the project is built in `RELEASE` mode.
To build the project in `DEBUG` mode (i.e. the executable and library include debuggin symbols and are compiled without optimizations) replace the `cmake ..` command with `cmake -DCMAKE_BUILD_TYPE=DEBUG ..`.
To cleanup from a build, you can simply delete the build directory.

The pairwise transient detection algorithm provides a specialized implementation that leverages SSE intrinsics.
This implementation requires that the target cpu supports the `ssse3` (not a typo) instruction set extensions.
This feature can be enabled via the `MELEX_VECTOR_BACKEND` environment variable; cpecify `MELEX_VECTOR_BACKEND=sse` before executing the `cmake..` command.
**NOTE:** This specialized implementation is *not* guaranteed to be faster than the implementation provided by auto-vectorization (especially when more recent vector instruction sets are available).


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

pymelex
-------

We provide a work-in-progress python package called pymelex to wrap the
library. This package supports python 3 and requires the numpy package. After
installation of the library, this package can installed with:

	# assuming you are in the root directory
	cd pymelex
	pip install -e .


Shared Library Usage
--------------------

To make use of the functionallity in this project, include melodyextraction.h
and tell the compiler to use libmelodyextraction.so. We also make the
libmelodyextraction_static.a static library available for use.
