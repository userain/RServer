#include <RServer_threadsPoll.h>
#include <stdlib.h>
//构造函数
    RServer_threadsPoll::RServer_threadsPoll(unsigned int nums,RServer_threadTaskQueue* queue,void*(*routine)(void *)){  
        if(nums == 0){
            cout << "error: poll threads nums is 0"<<endl;
        }
        if(pthread_mutex_init(&threadsPollLock,NULL)!=0){  //初始化锁
            cout << "init threadspoll mutex failed"<<endl;
            exit(1);
        }
        if(pthread_cond_init(&queueReadyCond,NULL)!=0){ // 初始化信号量
            cout << "init threadspoll cond failed"<<endl;
            exit(1);
        }
        
        pollStartStatus = true;
        taskQueue = queue;
        pthread_t pt;
        for(int i=0;i<nums;++i){          //创建线程池中的线程
            while(pthread_create(&pt,NULL,routine,NULL)!=0);        //启动的线程执行routine函数
            threadsList.push_back(pt);
        }
          
    }
    int RServer_threadsPoll::lock(){return pthread_mutex_lock(&threadsPollLock);}
    int RServer_threadsPoll::unLock(){return pthread_mutex_unlock(&threadsPollLock);}
    
    int RServer_threadsPoll::destoryPoll(){
        if(!pollStartStatus)
            return 0;
        pollStartStatus = false;
        lock();
        pthread_cond_broadcast(&queueReadyCond);            //唤醒所有线程
        unLock();
        //等待所有线程结束
        for(list<pthread_t>::iterator it = threadsList.begin();it!=threadsList.end();++it)
            pthread_join((*it),NULL);   

        //这个时候需要释放任务队列中的所有任务，这些任务虽然都没有执行，但是线程池已经退出，需要释放这些任务        
        while(taskQueue->getQueueLength()!=0){
            taskQueue->lockQueue();
            delete(taskQueue->getTask());
            taskQueue->unLockQueue();
        }
        
        while(pthread_mutex_destroy(&threadsPollLock)!=0);           //释放锁
        while(pthread_cond_destroy(&queueReadyCond)!=0);       //释放信号量
        return 0;
}    

    
    void RServer_threadsPoll::postSignal(){pthread_cond_signal(&queueReadyCond);}
    void RServer_threadsPoll::waitSignal(){
        pthread_cond_wait(&queueReadyCond,&threadsPollLock);
    }
    bool RServer_threadsPoll::getPollStartStatus(){return pollStartStatus;}
    
