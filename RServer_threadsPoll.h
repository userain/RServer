#include <list>
#include <pthread.h>
#include "RServer_threadTask.h"
using namespace std;

#ifndef _RServer_threadsPoll_H
#define _RServer_threadsPoll_H
class RServer_threadsPoll{
private:
    bool pollStartStatus;  
    list<pthread_t> threadsList;
    pthread_mutex_t threadsPollLock;
    RServer_threadTaskQueue *taskQueue;
    pthread_cond_t  queueReadyCond;
public:
    RServer_threadsPoll(unsigned int nums,RServer_threadTaskQueue* queue,void*(*routine)(void *));
    int lock();
    int unLock();
    int destoryPoll();
    void postSignal();
    void waitSignal();
    bool getPollStartStatus();
};

#endif