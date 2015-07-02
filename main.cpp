//在编译本程序时，一定要加上-pthread选项
//codelite设置 工程设置->global settings-》linker->options
#include <iostream>
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
#include "RServer_threadTask.h"
#include "RServer_threadsPoll.h"
#include "RServer_requestAcceptor.h"
using namespace std;

const int R_SERVERHTTPPORT = 9090;      //服务器端口
const char SERVERINFO[] = "Server: RServer1.0\r\n"; //服务器信息

void *threadRoutine(void *arg);
 
//全局多线程http请求任务队列，实际上是一个链表
RServer_threadTaskQueue taskQueue;   
                
//启动一个多线程池，共有20个线程，每个线程的启动函数为threadRoutine，任务队列为taskQueue  
RServer_threadsPoll myThreadsPoll(10,&taskQueue,threadRoutine);


/*****发送错误页面*****/
void badRequest(unsigned int connfd,RServer_requestMessage &badMessage){
    char html[1024];
    strcpy(html,"HTTP/1.1 404 request URI not found\r\n");
    send(connfd,html,strlen(html),0);
    strcpy(html,SERVERINFO);
    send(connfd,html,strlen(html),0);
    strcpy(html,"Content-Type: text/html\r\n");
    send(connfd,html,strlen(html),0);
    strcpy(html,"\r\n");
    send(connfd,html,strlen(html),0);
    strcpy(html,"<HTML><HEAD><TITLE>404 request URI not found\r\n</TITLE></HEAD>\r\n<BODY><P>404 request URI not found on server.\r\n</BODY></HTML>\r\n");
    send(connfd,html,strlen(html),0);
}

/*****发送unsupport页面*****/
void unsupportHtml(unsigned int connfd,RServer_requestMessage &unsupportMessage){
    char html[1024];
    strcpy(html,"HTTP/1.1 501 method is unsupported\r\n");
    send(connfd,html,strlen(html),0);
    strcpy(html,SERVERINFO);
    send(connfd,html,strlen(html),0);
    strcpy(html,"Content-Type: text/html\r\n");
    send(connfd,html,strlen(html),0);
    strcpy(html,"\r\n");
    send(connfd,html,strlen(html),0);
    strcpy(html,"<HTML><HEAD><TITLE>501 Method Not Implemented\r\n</TITLE></HEAD>\r\n<BODY><P>501 HTTP request method not supported.\r\n</BODY></HTML>\r\n");
    send(connfd,html,strlen(html),0);
}

/*****发送文件函数*****/
void sendHtml(int fd,FILE* resource){
    char buff[1024];
    while(fgets(buff,sizeof(buff),resource)!=NULL)
        send(fd,buff,strlen(buff),0);
}
/*****发送http回复头部分*****/
void sendServerHead(unsigned int connfd,RServer_requestMessage &okmessage){
    char html[1024];
    strcpy(html,"HTTP/1.1 200 OK\r\n");
    send(connfd,html,strlen(html),0);
    strcpy(html,SERVERINFO);
    send(connfd,html,strlen(html),0);
    strcpy(html,"Content-Type: text/html\r\n");
    send(connfd,html,strlen(html),0);
    strcpy(html,"\r\n");
    send(connfd,html,strlen(html),0);
}

//get方法处理函数
void getMethodFun(unsigned int connfd, RServer_requestMessage &getMessage){
    char htmlPath[100];
    //获取get请求中的文件地址
    if(getMessage.getRequestURI() == ""||getMessage.getRequestURI() == "/"){
        strcpy(htmlPath,"web/index.html");
    }else{
        strcpy(htmlPath,"web");
        string path = getMessage.getRequestURI();
        int len = path.length();
        char*requestPath= (char*)malloc(len*sizeof(char));
        path.copy(requestPath,len,0);
        strcat(htmlPath,requestPath);
        delete(requestPath);
    }
    //打开文件并发送
    FILE* htmlRecource = fopen(htmlPath,"r");
    if(htmlRecource == NULL){
        badRequest(connfd,getMessage);             //若不存在该文件，则调用badRequest(getMessage),返回错误页面
        return;
    }
    sendServerHead(connfd,getMessage);
    sendHtml(connfd,htmlRecource);
    fclose(htmlRecource);       //关闭文件描述符
}

//Post方法要调用CGI，暂时不支持
void postMethodFun(unsigned int fd,RServer_requestMessage &getMessage){
    
}
//这是线程池每个线程启动后的执行函数
void *threadRoutine(void *arg){
    while(1){
        myThreadsPoll.lock();               //在等待信号之前一定要先加锁，若不加锁会出现接收器发信号量了，但是这边错过信号量的情况，详细见prhread_cond_wait()函数使用说明
//若任务队列中没有任务，则等待信号量，此时线程就会进入睡眠状态，这里要用while是防止线程被意外唤醒的情况,并且一定要判断线程池的状态，若线程池是停止状态，表示这个信号是一个销毁线程池信号量，要退出该线程
         while(myThreadsPoll.getPollStartStatus()&&taskQueue.getQueueLength()==0)    
            myThreadsPoll.waitSignal();         //等待信号量
        myThreadsPoll.unLock();
        
        if(!myThreadsPoll.getPollStartStatus())               //如果是销毁线程池的信号量，则退出线程
            pthread_exit(NULL);
            
        RServer_threadTask* mytask=NULL; 
        if(taskQueue.lockQueue()!=0)
            cout << "error: get queue lock failed when gettask"<<endl;
        else{
            mytask = taskQueue.getTask();
            taskQueue.unLockQueue();
        }
        if(mytask == NULL)
            continue;
        
        //cout <<"start processfun"<<endl;
        //cout << mytask->getProcessFun()<<endl;
        //cout << mytask->getFunArgs()<<endl;
        mytask->getProcessFun()(mytask,mytask->getFunArgs());                 //启动任务处理函数
        //cout << "end processfun" << endl;
        delete(mytask);                         //这里很重要，一定要delete掉这个任务
    }
    return NULL;
}

//这是每一个任务所对应的处理函数，由于这是一个http服务器，所以对于每一个任务都是采用该函数解析请求，并传送html页面内容
//本服务器暂时只支持http中的get请求，由于post请求要调用cgi，比较麻烦,暂时不支持
void RServer_HandleRequest(RServer_threadTask* task,void *arg){
    RServer_requestMessage handleMessage = task->getMessage();
    //cout << "message method:"<<task->getMessage().getRequestMethod()<<endl;
    //cout << "message uri:"<<handleMessage.getRequestURI()<<endl;
    if(handleMessage.getRequestMethod() == "GET")       //判断正在处理的请求的Method
        getMethodFun(task->getConnfd(),handleMessage);                    // 若为GET请求，则调用getMethodFun(handleMessage)函数
    else if(handleMessage.getRequestMethod() == "POST")
        postMethodFun(task->getConnfd(),handleMessage);                   // 若为POST请求，则调用postMethodFun(handleMessage)函数
    else
        unsupportHtml(task->getConnfd(),handleMessage);                   //该服务器暂不支持其他类型请求，若为其他求出，调用unsupportHtml(handleMessage)，返回unsupport页面
    //cout << task->getConnfd()<< "  success" << endl;
    close(task->getConnfd());               //这里一定要记得关闭socket连接
}



int main(){
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;                          //ipv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);           //地址为0.0.0.0
    servaddr.sin_port = htons(R_SERVERHTTPPORT);                //监听端口为R_SERVERHTTPPORT
    RServer_requestAcceptor myAcceptor(servaddr,&taskQueue,&myThreadsPoll,RServer_HandleRequest);
    myAcceptor.start();
    return 1;
}