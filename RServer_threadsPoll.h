#include <list>
#include <pthread.h>
#include "RServer_threadTask.h"
using namespace std;

#ifndef _RServer_threadsPoll_H
#define _RServer_threadsPoll_H
class RServer_threadsPoll{
private:
    bool pollStartStatus;                   //线程池的启动状态
    list<pthread_t> threadsList;            //线程池中的每一个线程描述符
    pthread_mutex_t threadsPollLock;        //线程池中用于控制信号量访问的锁
    RServer_threadTaskQueue *taskQueue;     //线程池的中线程的任务队列
    pthread_cond_t  queueReadyCond;         //线程池中的任务到来条件变量
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
