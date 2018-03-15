#ifndef SIGOPT_H
#define SIGOPT_H
typedef struct sigOpt sigOpt;
typedef struct sigOptChannel sigOptChannel;

sigOptChannel *sigOptChannelCreate(int variable, int winSize, int hopsize,
				   int startIndex, int leftOffset,
				   int bufferLength, float scaleFactor);
void sigOptChannelDestroy(sigOptChannel *sOC);
#endif /* SIGOPT_H */
