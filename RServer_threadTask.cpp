#include "RServer_threadTask.h"

//RServer_requestMessage类实现
string RServer_requestMessage::getRequestMethod(){ return this->requestMethod; }
void   RServer_requestMessage::setRequestMethod(string method){requestMethod = method;}
string RServer_requestMessage::getRequestURI(){ return this->requestURI; }
void   RServer_requestMessage::setRequestURI(string uri){requestURI = uri;}
bool   RServer_requestMessage::setRequestMessage(string request){
    //切记这里传进来的请求字符串没有末尾空行
    if (request.empty())
    {
        cout << "error:no request line" << endl;
        return false;
    }
    //寻找请求方式
    size_t i = request.find(' ');
    if (i >= 0 && i < request.length())
    {
        this->requestMethod = request.substr(0, i);
        ++i;
        request = request.substr(i, request.length() - i);
    }else
        return false;
    //寻找请求URI
    i = request.find(' ');
    if (i >= 0 && i < request.length())
    {
        this->requestURI = request.substr(0, i);
        ++i;
        request = request.substr(i, request.length() - i);
    }else
        return false;
    return true;
}
RServer_requestMessage::RServer_requestMessage():requestMethod(""),requestURI(""){}

//RServer_threadTask类实现
unsigned int RServer_threadTask::getConnfd(){return connfd;}
void   RServer_threadTask::setConnfd(unsigned int fd){connfd = fd;}
RServer_requestMessage RServer_threadTask::getMessage(){return message;}
void RServer_threadTask::setMessage(RServer_requestMessage msg){message = msg;}
void (*RServer_threadTask::getProcessFun())(RServer_threadTask*,void*){return processFun;}
void* RServer_threadTask::getFunArgs(){return args;}
RServer_threadTask* RServer_threadTask::getNext(){return next;}
void   RServer_threadTask::setNext(RServer_threadTask* nextMessage){next =nextMessage;}
//构造函数
RServer_threadTask::RServer_threadTask(unsigned int fd,string request,void (*fun)(RServer_threadTask*,void*),void* funArgs){
    connfd = fd;
    message.setRequestMessage(request);
    processFun = fun;
    args = funArgs;
    next = NULL;
}

//RServer_threadTaskQueue类实现
RServer_threadTaskQueue::RServer_threadTaskQueue(){
    head = NULL;
    rear = head;
    length = 0;
    while(pthread_mutex_init(&queueLock,NULL)!=0);        //对队列锁进行初始化，直到成功为止
}
RServer_threadTaskQueue::~RServer_threadTaskQueue(){
    while(pthread_mutex_destroy(&queueLock)!=0);           //释放所资源，直至成功
}
int RServer_threadTaskQueue::lockQueue(){
    return pthread_mutex_lock(&queueLock);    
}
int RServer_threadTaskQueue::unLockQueue(){
    return pthread_mutex_unlock(&queueLock);
}
RServer_threadTask* RServer_threadTaskQueue::getTask(){
    if(length == 0)
        return NULL;
    RServer_threadTask *result = head;
    head=(head->getNext());
    --length;
    return result;
}
bool RServer_threadTaskQueue::insertTask(RServer_threadTask* req){
    if(length == 0){
        head = req;
        rear = req;
    }else{
        (*rear).setNext(req);
        rear = (*rear).getNext();
    }
    ++length;
    return true;
}
int RServer_threadTaskQueue::getQueueLength(){return length;}

