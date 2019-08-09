#include "melodyextraction.h"

int ReadAudioFile(char* inFile, float** buf, audioInfo* info, int verbose);
void SaveAsWav(const double* audio, audioInfo info, const char* path);
