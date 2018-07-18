#include "threadPoolInterface.h"
#include "thpool.h"

threadPool* threadPoolNew(int numThreads){
  return thpool_init(numThreads);
}

void threadPoolDestroy(threadPool* tP){
  thpool_destroy(tP);
}


int threadPoolAddJob(threadPool* tP, taskFunc func_ptr, void* arg_ptr){
  return thpool_add_work(tP,func_ptr, arg_ptr);
}

void threadPoolWait(threadPool *tP){
  thpool_wait(tP);
}
