#ifndef _HTTPSER_H_
#define _HTTPSER_H_
#include <iostream>
#include<cstring>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
using namespace std;


#define BUFFER_SIZE 4096
class  HttpSer{
    private:
        int socketfd; //套接字描述符
        string strbuf;//保存已经接收但是未处理的数据
        bool is_close;//是否关闭套接字
    public:
        HttpSer(){}
        HttpSer(int id,string buf1):socketfd(id),strbuf(buf1){is_close=false;}
        ~HttpSer(){}
        string processHttp();
        void Not_Implemented(string method);//非post,get方法
        void Not_Found(string url,string method);//文件未找到
        void response_get(string url,string method);//处理get方法
        void response_post(string name,string id);//处理post方法
        bool get_is_close(){return is_close;}
        int get_fd(){return socketfd;}
};

#endif