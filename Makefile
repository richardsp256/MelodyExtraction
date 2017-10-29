CC = gcc
CFLAGS = -Wall -O3 -fPIC
LD = gcc
LDFLAGS = -Wall


all: tuningAdjustment.o midi.o comparison.o pitchStrat.o onsetStrat.o onsetsds.o silenceStrat.o fVADsd.o HPSDetection.o BaNaDetection.o findpeaks.o findCandidates.o candidateSelection.o lists.o winSampleConv.o noteCompilation.o melodyextraction.o simpleDetFunc.o gammatoneFilter.o filterBank.o testOnset.o main.o 
	gcc tuningAdjustment.o midi.o comparison.o pitchStrat.o onsetStrat.o onsetsds.o silenceStrat.o fVADsd.o HPSDetection.o BaNaDetection.o findpeaks.o findCandidates.o candidateSelection.o lists.o winSampleConv.o noteCompilation.o melodyextraction.o simpleDetFunc.o gammatoneFilter.o filterBank.o testOnset.o main.o -o extract -L./fftw-3.3.4/.libs -lfftw3f -L./libsndfile-1.0.26/src/.libs -lsndfile -L./libfvad-master/src/.libs -Wl,-rpath=./libfvad-master/src/.libs -lfvad -L./libsamplerate-0.1.9/src/.libs -lsamplerate -lm

lib: tuningAdjustment.o midi.o comparison.o pitchStrat.o onsetStrat.o onsetsds.o silenceStrat.o fVADsd.o HPSDetection.o BaNaDetection.o findpeaks.o findCandidates.o candidateSelection.o lists.o winSampleConv.o noteCompilation.o melodyextraction.o simpleDetFunc.o gammatoneFilter.o filterBank.o testOnset.o
	gcc -shared tuningAdjustment.o midi.o comparison.o pitchStrat.o onsetStrat.o onsetsds.o silenceStrat.o fVADsd.o HPSDetection.o BaNaDetection.o findpeaks.o findCandidates.o candidateSelection.o lists.o winSampleConv.o noteCompilation.o melodyextraction.o simpleDetFunc.o gammatoneFilter.o filterBank.o testOnset.o -o libmelodyextraction.so -L./fftw-3.3.4/.libs -lfftw3f -L./libsndfile-1.0.26/src/.libs -lsndfile -L./libfvad-master/src/.libs -Wl,-rpath=./libfvad-master/src/.libs -lfvad -L./libsamplerate-0.1.9/src/.libs -lsamplerate -lm -fPIC

simpleDetFunc.exe: simpleDetFunc.o gammatoneFilter.o filterBank.o
	gcc simpleDetFunc.o gammatoneFilter.o filterBank.o -lm -L./libsamplerate-0.1.9/src/.libs -lsamplerate -o simpleDetFunc.exe

simpleDetFunc.o: simpleDetFunc.c
	${CC} ${CFLAGS} -c $< -o simpleDetFunc.o

gammatoneFilter.o: gammatoneFilter.c
	${CC} ${CFLAGS}  -I./libsamplerate-0.1.9/src -c $< -o gammatoneFilter.o

filterBank.o: filterBank.c
	${CC} ${CFLAGS} -c $< -o filterBank.o

testOnset.o: testOnset.c
	${CC} ${CFLAGS} -c $< -o testOnset.o

melodyextraction.o: melodyextraction.c
	${CC} ${CFLAGS} -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -c $< -o melodyextraction.o

tuningAdjustment.o: tuningAdjustment.c
	${CC} ${CFLAGS} -c $< -o tuningAdjustment.o

noteCompilation.o: noteCompilation.c
	${CC} ${CFLAGS} -c $< -o noteCompilation.o

winSampleConv.o: winSampleConv.c
	${CC} ${CFLAGS} -c $< -o winSampleConv.o

pitchStrat.o: pitchStrat.c
	${CC} ${CFLAGS} -c $< -o pitchStrat.o

onsetStrat.o: onsetStrat.c
	${CC} ${CFLAGS} -I./libsndfile-1.0.26/src -I./libsamplerate-0.1.9/src -c $< -o onsetStrat.o

onsetsds.o: onsetsds.c
	${CC} ${CFLAGS} -c $< -o onsetsds.o

fVADsd.o: fVADsd.c
	${CC} ${CFLAGS} -I./libsamplerate-0.1.9/src -I./libfvad-master/include -c $< -o fVADsd.o

silenceStrat.o: silenceStrat.c
	${CC} ${CFLAGS} -I./libfvad-master/include -c $< -o silenceStrat.o

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

comparison.o: comparison.c
	${CC} -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api ${CFLAGS} -c $< -o comparison.o

midi.o: midi.c
	${CC} -I./libsndfile-1.0.26/src ${CFLAGS} -c $< -o midi.o

main.o: main.c
	${CC} -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api ${CFLAGS} -c $< -o main.o

clean:
	rm -f *.o
