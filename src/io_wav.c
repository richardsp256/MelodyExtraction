#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "io_wav.h"
#include "sndfile.h"
#include "errors.h"

int ReadAudioFile(char* inFile, float** buf, audioInfo* info, int verbose)
{
	SF_INFO file_info;
	SNDFILE * f = sf_open(inFile, SFM_READ, &file_info);
	if( !f ){
		return ME_INVALID_FILE;
	}
	if(file_info.channels != 1){
		sf_close( f );
		return ME_FILE_NOT_MONO;
	}

	if (verbose){
		printf("Frames:\t%ld\n", file_info.frames);
		printf("Sample rate:\t%d\n", file_info.samplerate);
		printf("Channels: \t%d\n", file_info.channels);
		printf("Format: \t%d\n", file_info.format);
		printf("Sections: \t%d\n", file_info.sections);
		printf("Seekable: \t%d\n", file_info.seekable);
	}

	// Copy relevant information from file_info into info
	info->frames = file_info.frames;
	info->samplerate = file_info.samplerate;

	(*buf) = malloc( sizeof(float) * file_info.frames);
	sf_readf_float( f, (*buf), file_info.frames );
	sf_close( f );

	return ME_SUCCESS;
}

void SaveAsWav(const double* audio, audioInfo info, const char* path) {
	FILE* file = fopen(path, "wb");

	//Encode samples to 16 bit
	short* samples = (short*)malloc(info.frames* sizeof(short));
	unsigned int i;
	short max = -10000;
	int maxloc = 0;
	short min = 10000;
	int minloc = 0;
	for (i = 0; i < info.frames; ++i) {
		samples[i] = htole16(fmin(fmax(audio[i], -1), 1) * SHRT_MAX);
		if(samples[i] > max){
			max = samples[i];
			maxloc = i;
		}
		if(samples[i] < min){
			min = samples[i];
			minloc = i;
		}
	}

	printf("\tmax: %d at %d\tmin: %d at %d\n", max, maxloc, min, minloc);

	//Heaader chunk
	fprintf(file, "RIFF");
	unsigned int chunksize = htole32((info.frames * sizeof(short)) + 36);
	fwrite(&chunksize, sizeof(chunksize), 1, file);
	fprintf(file, "WAVE");

	//Format chunk
	fprintf(file, "fmt ");
	unsigned int fmtchunksize = htole32(16);
	fwrite(&fmtchunksize, sizeof(fmtchunksize), 1, file);
	unsigned short audioformat = htole16(1);
	fwrite(&audioformat, sizeof(audioformat), 1, file);
	unsigned short numchannels = htole16(1);
	fwrite(&numchannels, sizeof(numchannels), 1, file);
	unsigned int samplerate = htole32(info.samplerate);
	fwrite(&samplerate, sizeof(samplerate), 1, file);
	unsigned int byterate = htole32(samplerate * numchannels * sizeof(short));
	fwrite(&byterate, sizeof(byterate), 1, file);
	unsigned short blockalign = htole16(numchannels * sizeof(short));
	fwrite(&blockalign, sizeof(blockalign), 1, file);
	unsigned short bitspersample = htole16(sizeof(short) * CHAR_BIT);
	fwrite(&bitspersample, sizeof(bitspersample), 1, file);

	//Data chunk
	fprintf(file, "data");
	unsigned int datachunksize = htole32(info.frames * sizeof(short));
	fwrite(&datachunksize, sizeof(datachunksize), 1, file);
	fwrite(samples, sizeof(short), info.frames, file);

	//Free encoded samples
	free(samples);

	//Close the file
	fclose(file);
}
