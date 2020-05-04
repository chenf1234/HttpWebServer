#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <pthread.h>
#include <queue>
#include <vector>
#include <map>
#include "HttpSer.h"
#include <exception>
#include <sys/epoll.h>
using namespace std;

class Threadpool{
    private:
        pthread_mutex_t lock;    //互斥锁
        pthread_cond_t queue_cond;   //唤醒消息队列等待线程
        queue<HttpSer> work_queue; //要处理的套接字工作队列
        int thread_num; //线程数
        vector<pthread_t> thread;  //线程池中的线程
    
    private:
        map<int,string> buf;//连接套接字以及相应保存数据的缓冲区
        map<int,int> vis;
       

    public:
        //vector<int> delsock;//要从epoll中移除的套接字
        Threadpool(int num):thread_num(num){
            pthread_mutex_init(&lock,NULL);
            queue_cond = PTHREAD_COND_INITIALIZER;
            thread=vector<pthread_t>(thread_num);
        }

        ~Threadpool(){
            pthread_mutex_destroy(&lock);
            pthread_cond_destroy(&queue_cond);
        }
        void start();//创建线程

        //借助生产者消费者问题思想,queue_append是生产者,run是消费者
        void queue_append(int fd);//向工作队列中加入http请求
        void queue_consumer();//消费工作队列中的http请求
        static void* work(void *arg);//线程创建后的执行函数
        void init_fd(int clntfd); //初始化buf和vis
    
      
};

#endif