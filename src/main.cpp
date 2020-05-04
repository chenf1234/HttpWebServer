#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>

#include "Threadpool.h"

int sersocket;
int epfd;

void closesocket(int sig){
    if(sig==SIGINT){
        cout<<"close server socket"<<endl;
        close(sersocket);
        close(epfd);
    }
}//利用信号机制关闭服务端套接字

void setnonblockingmode(int fd){//将socket设置成非阻塞
    int flag =  fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flag|O_NONBLOCK);
}

int WebServer(const char* ip,int port,int thread){
    sersocket=socket(PF_INET,SOCK_STREAM,0);
   signal(SIGINT,closesocket);
    if(sersocket < 0){
		printf("socket error\n");
		exit(1);
    }
    const int size=10*thread;
    int option=1;
    socklen_t optlen=sizeof(option);
    setsockopt(sersocket,SOL_SOCKET,SO_REUSEADDR,(void*)&option,optlen);

    struct sockaddr_in servaddr,clnt_addr;
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(port);
    servaddr.sin_addr.s_addr=inet_addr(ip);

    if(bind(sersocket,(struct sockaddr*)&servaddr,sizeof(servaddr))==-1){
        printf("bind() error\n");
        exit(1);
    }

    if(listen(sersocket,size)==-1){
        printf("listen() error\n");
        exit(1);
    }
    
    setnonblockingmode(sersocket);

    Threadpool pool(thread);
    pool.start();
   

    
    epfd=epoll_create(size);//将大小设置为和listen第二个参数一样
    struct epoll_event *ep_events;
    struct epoll_event event;
    ep_events=new epoll_event[size];
    event.events=EPOLLIN;
    event.data.fd=sersocket;
    epoll_ctl(epfd,EPOLL_CTL_ADD,sersocket,&event);

    while(1){

        int event_cnt=epoll_wait(epfd,ep_events,size,-1);
        if(event_cnt==-1){
            printf("epoll_wait() error\n");
            exit(1);
        }
        for(int i=0;i<event_cnt;i++){
            if(ep_events[i].data.fd==sersocket){
                socklen_t adr_sz=sizeof(clnt_addr);
                int clntsocket=accept(sersocket,(struct sockaddr*)&clnt_addr,&adr_sz);
                // time_t t = time(0);
                // char tmp[64];
                // strftime(tmp, sizeof(tmp), "%Y/%m/%d %X", localtime(&t));
                // cout << tmp <<" : "<<clntsocket<< endl;
                if(clntsocket!=-1){
                    setnonblockingmode(clntsocket);
                    event.events=EPOLLIN|EPOLLET;//边缘触发
                    event.data.fd=clntsocket;
                    
                    if(epoll_ctl(epfd,EPOLL_CTL_ADD,clntsocket,&event)==-1){
                        // strftime(tmp, sizeof(tmp), "%Y/%m/%d %X", localtime(&t));
                        // cout << tmp <<" : "<<"error"<< endl;
                    }
                    pool.init_fd(clntsocket);
                   
                }
            }
            else{
                pool.queue_append(ep_events[i].data.fd);
            }
        }
    }
    // close(sersocket);
}

int main(int argc,char* argv[]){
    int opt;//getopt_long返回值
    const char *optstring="i:p:n:";
    struct option longopt1[]={
        {"ip",1,NULL,'i'},
        {"port",1,NULL,'p'},
        {"number-thread",1,NULL,'n'},
        {0,0,0,0}
    };
    const char *ip="127.0.0.1";
    int port=8989;
    int numThread=10;
    while((opt=getopt_long(argc,argv,optstring,longopt1,NULL))!=-1){
        switch(opt){
            case 'i':{
                ip=optarg;
                break;
            }
            case 'p':{
                port=atoi(optarg);
                break;
            }
            case 'n':{
                numThread=atoi(optarg);
                break;
            }
            case '?':{
                printf("unknown option\n");
                exit(1);
                break;
            }
            default:break;
        }
    }
    printf("ip : %s\nport : %d\nnumthread : %d\n ",ip,port,numThread);
    WebServer(ip,port,numThread);
    
    return 0;
}