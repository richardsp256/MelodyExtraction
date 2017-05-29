CC = gcc
CFLAGS = -Wall
LD = gcc
LDFLAGS = -Wall

all: comparison.c comparison.h midi.c midi.h main.c detectionStrat.o HPSDetection.o BaNaDetection.o findpeaks.o findCandidates.o candidateSelection.o candidates.o lists.o peakQueue.o
	gcc -Wall -O3 -c midi.c -o midi.o
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c comparison.c -o comparison.o
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c main.c -o main.o
	gcc midi.o comparison.o detectionStrat.o HPSDetection.o BaNaDetection.o findpeaks.o findCandidates.o candidateSelection.o candidates.o lists.o peakQueue.o main.o -o extract -L./fftw-3.3.4/.libs -lfftw3 -L./libsndfile-1.0.26/src/.libs -lsndfile -lm

detectionStrat.o: detectionStrat.c
	${CC} ${CFLAGS} -c $< -o detectionStrat.o

HPSDetection.o: HPSDetection.c
	${CC} ${CFLAGS} -c $< -o HPSDetection.o

BaNaDetection.o: BaNaDetection.c
	${CC} ${CFLAGS} -c $< -o BaNaDetection.o

candidateSelection.o: candidateSelection.c
	${CC} ${CFLAGS} -c $< -o candidateSelection.o

findpeaks.o: findpeaks.c
	${CC} ${CFLAGS} -c $< -o findpeaks.o

peakQueue.o: peakQueue.c
	${CC} ${CFLAGS} -c $< -o peakQueue.o

findCandidates.o: findCandidates.c
	${CC} ${CFLAGS} -c $< -o findCandidates.o

lists.o: lists.c
	${CC} ${CFLAGS} -c $< -o lists.o

candidates.o: candidates.c
	${CC} ${CFLAGS} -c $< -o candidates.o

clean:
	rm *.o
