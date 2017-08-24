float FractionalAverage(float* arr, int len, int centerInd, float* avg);
float fractPart(float x);
float weightDist(float a, float b);
float squareDistWrapped(float* arr, int len, float pt);
float squareDistWrappedWeighted(float* arr, int len, float pt, float* weights);
float mean(float* arr, int len);
float meanWeighted(float* arr, int len, float* weights);
float min(float* arr, int len);
float sum(float* arr, int len);