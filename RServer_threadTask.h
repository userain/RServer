#include <string>
#include <iostream>
#include <pthread.h>
using namespace std;

#ifndef _RServer_threadPoll_H
#define _RServer_threadPoll_H

class RServer_requestMessage{
private:
	string requestMethod;			//请求方法
	string requestURI;			//请求资源的URL
    //string content;           //现如今只处理get方法，所以没有写这一部分
public:
    string getRequestMethod();
    void   setRequestMethod(string method);
    string getRequestURI();
    void   setRequestURI(string uri);
    bool   setRequestMessage(string request);
    RServer_requestMessage();
};

//线程任务类
class RServer_threadTask{
private:
    unsigned int connfd;
    RServer_requestMessage message;
    void (*processFun)(RServer_threadTask*,void*);
    void * args;
    RServer_threadTask *next;
public:
    unsigned int getConnfd();
    void   setConnfd(unsigned int fd);
    RServer_requestMessage getMessage();
    void setMessage(RServer_requestMessage msg);
    void (*getProcessFun())(RServer_threadTask*,void*);
    void* getFunArgs();
    RServer_threadTask* getNext();
    void   setNext(RServer_threadTask* nextMessage);
    //构造函数
    RServer_threadTask(unsigned int fd,string request,void (*fun)(RServer_threadTask*,void*),void* funArgs);
};

//多线程任务队列类
class RServer_threadTaskQueue{
private:
    RServer_threadTask* head;
    RServer_threadTask* rear;
    unsigned int length;
    pthread_mutex_t queueLock;         //注意这里进行对请求队列使用了读写锁机制
public:
    RServer_threadTaskQueue();
    ~RServer_threadTaskQueue();
    int lockQueue();
    int unLockQueue();
    
    RServer_threadTask* getTask();
    
    bool insertTask(RServer_threadTask* req);
    int getQueueLength();
};

#endif