/*
libfvad.so, libfvad.la, libdvad.so.0, and libfvad.so.0.0.0 are installed in /usr/local/lib

fvad.h is installed in /usr/local/include
*/

#include <fvad.h>
#include <stdlib.h>

int libfvadSilenceDetection(float** AudioData,int sample_rate, int mode,
			    int frameLength, int length, int* musicFrames);
void convertSamples(float *inputData, int start, int frameLength,
		    int16_t *buffer);
int vadHelper(float* data,int sample_rate, int mode, int frameLength,
	      int length, int* musicFrames);


int libfvadSilenceDetection(float** AudioData,int sample_rate, int mode,
			    int frameLength, int length, int* musicFrames)
{
	// this function determines the entries of musicFrames and returns the
	// length of musicFrames

	// frameLength has units of milliseconds
	// check that it has values of 10, 20, or 30

	// check that mode can only be 0, 1, 2, or 3

	int musicFramesLength;
	
	// Here we will check the sample_rate
	// The only allowed sample_rate values are 8000,16000,32000,48000
	
	if (sample_rate != 8000 && sample_rate != 16000 && sample_rate != 32000
	    && sample_rate != 48000) {
		// we will resample. Since higher rates are resampled down to
		// 8000 within the program, we will just resample down to 8000

		// should probably check that sample rate is higher than 8000

		// I will probably use Secret Rabbit Code (aka libsamplerate)
		// to start - it currently has a 2-clause BSD license
		// http://www.mega-nerd.com/SRC/
		SRC_DATA *resampleData = malloc(sizof(SRC_DATA));
		resampleData -> data_in = (*AudioData);
		// should try to calculate the max number of frames
		resampleData -> data_out = malloc(sizeof(float) * length);
		// the following 2 values are set under the assumption that we
		// only have 1 channel
		resampleData -> input_frames = (long)length;
		resampleData -> output_frames = (long)length;
		resampleData -> src_ratio = 8000./((double)sample_rate);

		// I do not need need to set up input_frames_used,
		// output_frames_used, end_of_input

		// the second argument in src_simple corresponds to the
		// converter_type. By default I have set it to the best
		// quality
		int success_code;
		success_code = src_simple(resampleData, 0, 1);
		effectiveSampleRate = 8000;
		effectiveLength = resampleData -> output_frames;
		musicFramesLength = vadHelper(resampleData -> data_out,
					      8000, mode, frameLength,
					      resampleData -> output_frames,
					      musicFrames);
		
		free(resampleData -> data_out);
		free(resampleData);
	} else {
		musicFramesLength = vadHelper((*AudioData), sample_rate, mode,
					      frameLength, length, musicFrames);
	}

	return musicFramesLength;
}




void convertSamples(float *inputData, int start, int frameLengthSamples,
		    int16_t *buffer)
{
	// Converts Audio samples from floats to ints
	// I am following the example from libfvad
	for (int i = 0; i < frameLength){
		buffer[i] = (int16_t)(inputData[start + i] * INT16_MAX);
		i++;
	}
}


int vadHelper(float* data,int sample_rate, int mode, int frameLength,
	      int length, int* musicFrames){
	// this function actually runs VAD
	// conver should be converted to number of samples

	// this function determines the entries of musicFrames and returns the
	// length of musicFrames
	
	Fvad *vad;
	int16_t *buffer;
	int i, j, musicFrameLength, frameLengthSamples;

	// frameLengthSamples is just the frameLength in units of samples
	frameLengthSamples = (frameLength * sample_rate)/1000;
	
	buffer = malloc(sizeof(int16_t) * frameLengthSamples);
	
	musicFrameLength = length/frameLengthSamples;
	if (frameLengthSamples*musicFrameLength != length){
		musicFramesLength++;
	}

	// intialize a vad instance
	vad = fvad_new();

	fvad_set_mode(vad,mode);
	fvad_set_sample_rate(vad,sample_rate);

	j = 0;
	for (i=0;i<(length-frameLengthSamples);i+=frameLengthSamples){
		// copy to buffer and convert to int16
		convertSamples(data, i, frameLengthSamples,buffer);
		
		// process the data in the buffer
		musicFrames[j] = fvad_process(vad, buffer, frameLengthSamples);
		j++;
	}

	// now we just need to handle the last frame
	
	// for now, if length is not evenly divisible by frameLengthSamples, I 
	// will use the samples from the previous frame
	convertSamples(data, length-frameLengthSamples, frameLengthSamples,
		       buffer);
	musicFrames[j] = fvad_process(vad, buffer, frameLengthSamples);
	
	
	// clean up
	fvad_free(vad);
	free(buffer);
	return musicFramesLength;
}
