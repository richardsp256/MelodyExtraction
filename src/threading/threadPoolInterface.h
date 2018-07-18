/* The whole idea of implementing threadPoolInterface is so that we can 
 * easilly swap the actual thread pool implementation */

#include "thpool.h"

typedef thpool_ threadPool;
typedef void (*taskFunc)(void* arg_ptr);

threadPool* threadPoolNew(int numThreads);

void threadPoolDestroy(threadPool* tP);

int threadPoolAddJob(threadPool* tP, taskFunc func_ptr, void* arg_ptr);

void threadPoolWait(threadPool* tP);
