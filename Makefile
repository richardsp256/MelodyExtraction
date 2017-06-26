CC = gcc
CFLAGS = -Wall
LD = gcc
LDFLAGS = -Wall

all: comparison.c comparison.h midi.c midi.h main.c pitchStrat.o onsetStrat.o onsetsds.o silenceStrat.o fVADsd.o HPSDetection.o BaNaDetection.o findpeaks.o findCandidates.o candidateSelection.o lists.o 
	gcc -Wall -O3 -c midi.c -o midi.o
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c comparison.c -o comparison.o
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c main.c -o main.o
	gcc midi.o comparison.o pitchStrat.o onsetStrat.o onsetsds.o silenceStrat.o fVADsd.o HPSDetection.o BaNaDetection.o findpeaks.o findCandidates.o candidateSelection.o lists.o main.o -o extract -L./fftw-3.3.4/.libs -lfftw3f -L./libsndfile-1.0.26/src/.libs -lsndfile -lm -L/usr/local/lib -lfvad -lsamplerate

pitchStrat.o: pitchStrat.c
	${CC} ${CFLAGS} -c $< -o pitchStrat.o

onsetStrat.o: onsetStrat.c
	${CC} ${CFLAGS} -c $< -o onsetStrat.o

onsetsds.o: onsetsds.c
	${CC} ${CFLAGS} -c $< -o onsetsds.o

fVADsd.o: fVADsd.c
	${CC} ${CFLAGS} -c $< -o fVADsd.o

silenceStrat.o: silenceStrat.c
	${CC} ${CFLAGS} -c $< -o silenceStrat.o

HPSDetection.o: HPSDetection.c
	${CC} ${CFLAGS} -c $< -o HPSDetection.o

BaNaDetection.o: BaNaDetection.c
	${CC} ${CFLAGS} -c $< -o BaNaDetection.o

candidateSelection.o: candidateSelection.c
	${CC} ${CFLAGS} -c $< -o candidateSelection.o

findpeaks.o: findpeaks.c
	${CC} ${CFLAGS} -c $< -o findpeaks.o

findCandidates.o: findCandidates.c
	${CC} ${CFLAGS} -c $< -o findCandidates.o

lists.o: lists.c
	${CC} ${CFLAGS} -c $< -o lists.o

clean:
	rm *.o
