#include "Threadpool.h"
#include <algorithm>
#include <iostream>
#include <sys/epoll.h>


using namespace std;

void Threadpool::start(){
    //所有子线程创建完成后，子线程才开始运行
    pthread_mutex_lock(&lock);
    for(int i=0;i<thread_num;i++){

        //当某个线程创建失败时，一直重复创建线程，直到成功
        while(pthread_create(&thread[i],NULL,work,this)!=0);

        //将线程设置为脱离线程，失败则清除成功申请的资源并抛出异常
        if(pthread_detach(thread[i])){
            throw std::exception();
        }
    }
    pthread_mutex_unlock(&lock);

}

void* Threadpool::work(void* arg){
    Threadpool* pool=(Threadpool*) arg;
    pool->queue_consumer();
    return NULL;
}

void Threadpool::queue_consumer(){
    /***********借助生产者消费者问题思想*************/
    while(1){
        pthread_mutex_lock(&lock);
        while(work_queue.empty()){
            pthread_cond_wait(&queue_cond,&lock);
        }
        
        HttpSer http=work_queue.front();
        work_queue.pop();
        //无需唤醒生产者，生产者不休眠
        pthread_mutex_unlock(&lock);
        //调用HttpSer函数处理http请求
        //cout<<pthread_self()<<endl;
        string rbuf=http.processHttp();
        pthread_mutex_lock(&lock);
        if(http.get_is_close()){
            vis.erase(http.get_fd());
            buf.erase(http.get_fd());
           
        }
        else{
            vis[http.get_fd()]=0;
            buf[http.get_fd()]=rbuf;
        }
        pthread_mutex_unlock(&lock);
        
           
         

    }
}

void Threadpool::queue_append(int fd){
    pthread_mutex_lock(&lock);
    if(vis[fd]==0){
        HttpSer t(fd,buf[fd]);
        vis[fd]=1;
        work_queue.push(t);
        pthread_cond_signal(&queue_cond);//唤醒休眠线程
    }
    pthread_mutex_unlock(&lock);
}

void Threadpool::init_fd(int clntfd){
    pthread_mutex_lock(&lock);
    vis[clntfd]=0;
    buf[clntfd]="";
    pthread_mutex_unlock(&lock);
}
