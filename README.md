This tool is designed to analyze an audio file and output a MIDI 
representation of the melodic line.

This is a work-in-progress, with not all components implemented yet.

This project utilizes the FFTW and Libsndfile libraries. It was tested using
fftw-.3.4 and libsndfile-1.0.26

INSTALLATION
------------
Download and extract FFTW and Libsndfile project folder, then from both
library directories run the commands:	

	./configure
	make

Now that both libraries are compiled, you can compile and run the project.

If for any reason you wish to recompile these libraries, you must first remove
the current configuration files with
	
	make distclean

in that libraries directory.