#include "Threadpool.h"

/**
 *  pthread有两种状态joinable状态和unjoinable状态.
 * 如果线程是joinable状态，当线程函数自己返回退出时或pthread_exit时都不会释放线程
 * 所占用堆栈和线程描述符（总计8K多）。只有当你调用了pthread_join之后这些资源才会被释放。
 *  若是unjoinable状态的线程，这些资源在线程函数退出时或pthread_exit时自动会被释放。
 * unjoinable属性可以在pthread_create时指定，或在线程创建后在线程中pthread_detach自己, 
 * 如：pthread_detach(pthread_self())，将状态改为unjoinable状态，确保资源的释放。
 * 或者将线程置为 joinable,然后适时调用pthread_join.
 * 在Web服务器中当主线程为每个新来的链接创建一个子线程进行处理的时候，
 * 主线程并不希望因为调用pthread_join而阻塞,此时调用pthread_detach设置为脱离线程
 * */
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
        //cout<<"run"<<endl;
        HttpSer http=work_queue.front();
        work_queue.pop();
        //无需唤醒生产者，生产者不休眠
        pthread_mutex_unlock(&lock);
        //调用HttpSer函数处理http请求
        http.processHttp();
        //cout<<"over"<<endl;
    }
}

void Threadpool::queue_append(HttpSer t){
    pthread_mutex_lock(&lock);
    work_queue.push(t);
    pthread_cond_signal(&queue_cond);//唤醒休眠线程
    pthread_mutex_unlock(&lock);
}

