all: comparison.c comparison.h midi.c midi.h main.c
	gcc -Wall -O3 -c midi.c -o midi.o
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c comparison.c -o comparison.o
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c main.c -o main.o
	gcc midi.o comparison.o main.o -o extract -L./fftw-3.3.4/.libs -lfftw3 -L./libsndfile-1.0.26/src/.libs -lsndfile -lm
