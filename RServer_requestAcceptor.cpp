#include "RServer_threadTask.h"
#include "RServer_threadsPoll.h"
#include "RServer_requestAcceptor.h"
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>

//RServer_requestAcceptor类实现
//设置描述符sock为非阻塞式
void RServer_requestAcceptor::setnonblocking(int sock){
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)
    {
      perror("fcntl(sock,GETFL)");
      exit(1);
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
      perror("fcntl(sock,SETFL,opts)");
      exit(1);
    }  
}   
//构造函数
RServer_requestAcceptor::RServer_requestAcceptor(sockaddr_in serverAddr,RServer_threadTaskQueue* myTaskQueue,
                RServer_threadsPoll* myThreadsPoll,void(*myreqeustProcessFun)(RServer_threadTask*,void*)){
    memset(&listenAddr,0,sizeof(listenAddr));
    listenAddr.sin_family = serverAddr.sin_family;
    listenAddr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
    listenAddr.sin_port = serverAddr.sin_port;    
    taskQueue = myTaskQueue;
    threadsPoll = myThreadsPoll;
    reqeustProcessFun = myreqeustProcessFun;
}
//接收器启动函数
int RServer_requestAcceptor::start(){
    //cout << "begin start"<<endl;
    listenFd = socket(AF_INET,SOCK_STREAM,0);
    setnonblocking(listenFd);       //设置listenFd为非阻塞式的 
    bind(listenFd,(struct sockaddr*)&listenAddr,sizeof(listenAddr));        //绑定socket
    //cout << "start creat epoll"<<endl;
    epfd = epoll_create(MAX_LEN);
    struct epoll_event tempEvent;
    tempEvent.data.fd = listenFd;           //设置tempEvent的描述符为listenfd
    //tempEvent.events = EPOLLIN|EPOLLET;             //设置tempEvent要监听的事件为可读且为边沿触发
    tempEvent.events = EPOLLIN;                     //设置tempEvent要监听的事件为可读且为水平触发
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenFd,&tempEvent);//将tempEvent注册进epoll中
    listen(listenFd,5);             //监听，并设置监听队列长度为5
    
    cout << "***welcome RServer!***"<<endl;
    cout << "*********Rain*********"<<endl;
    cout << "*****start listen*****" <<endl; 
    
    int eventsNum = 0;
    int readLength = 0;
    char buff[MAX_BUFFLEN];
    unsigned int connFd;
    epoll_event events[MAX_LEN];
    struct sockaddr_in clientAddr;
    unsigned int clientLen=0;
    while(1){
        memset(buff,0,sizeof(buff)); 
        eventsNum = epoll_wait(epfd,events,MAX_LEN,-1);         //IO复用开始等待事件
        for(int i=0;i<eventsNum;++i){
            if(events[i].data.fd == listenFd){  //表示有新的连接请求
                memset(&clientAddr,0,sizeof(clientAddr));
                clientLen = sizeof(clientAddr);
                connFd = accept(listenFd,(struct sockaddr*)&clientAddr,&clientLen);           //接收请求
                if(connFd < 0){
                    cout << "Accept error! IP:"<<inet_ntoa(clientAddr.sin_addr);
                    cout <<"  port:"<<ntohs(clientAddr.sin_port);
                    cout <<"  Socket:"<<connFd<<endl;
                    continue;
                }
                //setnonblocking(connFd); 
                tempEvent.data.fd = connFd;
                //tempEvent.events = EPOLLIN|EPOLLET;             //设置新的连接要监听的事件为可读且为边沿触发
                tempEvent.events = EPOLLIN; 
                epoll_ctl(epfd,EPOLL_CTL_ADD,connFd,&tempEvent);
            }else if(events[i].events&EPOLLIN){         //如果是链接可读事件
                readLength = recv(events[i].data.fd,buff,MAX_BUFFLEN,0);
                if(readLength<0){
                    memset(&clientAddr,0,sizeof(clientAddr));
                    clientLen = sizeof(clientAddr);
                    getsockname(events[i].data.fd,(struct sockaddr*)&clientAddr,&clientLen);
                    cout << "Read error! IP:"<<inet_ntoa(clientAddr.sin_addr);
                    cout <<"  port:"<<ntohs(clientAddr.sin_port);
                    cout <<"  Socket:"<<events[i].data.fd<<endl;
                    continue;
                }
                else if(readLength>0){
                    buff[readLength] = '\0';
                    //cout<<endl<<events[i].data.fd <<":   "<< buff;             //打印请求
                    //RServer_requestMessage为请求信息类，创建一个请求信息对象并初始化,详见RServer_requestMessageQueue.h文件
                    
                    RServer_threadTask* newReqeust = new RServer_threadTask(events[i].data.fd,buff,reqeustProcessFun,NULL);  
                    //这里需要加锁
                    if(taskQueue->lockQueue()!=0){  //对请求队列taskQueue进行加锁，直至成功加锁为止
                        cout <<"error: get queue lock failed when insert task in queue"<<endl;
                        delete(newReqeust);             //若失败则delete newrequest
                        close(events[i].data.fd);       //并且一定要关闭socket
                    }
                    else{
                        if(!taskQueue->insertTask(newReqeust)){     //向请求队列末尾插入最新请求
                            close(newReqeust->getConnfd());
                            delete(newReqeust);             //若失败加入队列失败，要记得释放newRequest的空间
                        }
                        taskQueue->unLockQueue();      //对taskQueue进行解锁
                    
                        if(threadsPoll->lock()!=0)                //发送信号量之前一定要用锁
                            cout << "error: get cond lock failed"<<endl;
                        else{
                            threadsPoll->postSignal();         //发送信号量
                            threadsPoll->unLock();
                        }
                    }
                }
            }
        }
    }
}
//析构函数
RServer_requestAcceptor::~RServer_requestAcceptor(){
threadsPoll->destoryPoll();        //关闭线程池
close(listenFd);                //停止监听
close(epfd);                    //关闭epoll   
}