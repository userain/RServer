#include <sys/socket.h>
#include <fcntl.h>
#include "RServer_threadTask.h"
#include "RServer_threadsPoll.h"
#include <netinet/in.h>
#define MAX_BUFFLEN 1024
#define MAX_LEN 256
//接收器类，负责io复用接收http请求，然后将读取到的http请求加入到多线程任务队列中,发送信号量
class RServer_requestAcceptor{
private:
    unsigned int listenFd;
    unsigned int epfd;
    struct sockaddr_in listenAddr;
    RServer_threadTaskQueue *taskQueue;                  //任务队列  
    RServer_threadsPoll *threadsPoll;                  //线程池
    void (*reqeustProcessFun)(RServer_threadTask*,void*);
private:
  //设置描述符sock为非阻塞式
    void setnonblocking(int sock);
public:     
    RServer_requestAcceptor(sockaddr_in serverAddr,RServer_threadTaskQueue* myTaskQueue,
                    RServer_threadsPoll* myThreadsPoll,void(*myreqeustProcessFun)(RServer_threadTask*,void*));
    int start();            //接收器启动函数
  ~RServer_requestAcceptor();
};