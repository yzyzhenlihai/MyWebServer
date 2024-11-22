#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_
#include<iostream>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<string.h>
#include"epoller.h"
#include "../log/log.h"
#include"../http/httpconn.h"
#include"../pool/threadpool.h"
#include "../pool/sqlconnpool.h"
#include "../timer/timer.h"
#include<memory>//智能指针
#include<unordered_map>
#include<fcntl.h>
class WebServer{
public:
    WebServer(int port,int maxevents,int timeoutMS,int threadNum,
              int sqlPort,const char *sqlUserName,const char *sqlPwd,
              const char *dbName,int connNum,bool isLog);
    ~WebServer();
    void Start();//启动服务器
    
private:
    bool InitSocket();//初始化listenfd
    void InitEventMode();//初始化文件描述符的触发模式

    void SetFdNonblock(int fd);

    void DealListen();
    void DealRead(HttpConn *conn);
    void DealWrite(HttpConn *conn);
    void OnRead(HttpConn *conn);
    void OnWrite(HttpConn *conn);
    void OnProcess(HttpConn *conn);
    void CloseConn(HttpConn *conn);
    void SendError(int fd,const char *err);
    void ExtendTime(HttpConn *conn);//延长指定连接的定时器时间
    int listenFd_;//监听socket
    int port_;//端口号
    int timeoutMS_;//超时时间 毫秒
    bool isClose;
    char* srcDir_;//资源路径
    uint32_t listenEvent_;//监听fd的触发模式
    uint32_t connEvent_;//连接fd的触发模式

    std::unique_ptr<Epoller> epoll_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<Timer> timer_;
    std::unordered_map<int,HttpConn> users;//服务端记录每个连接的用户信息
    static const int MAX_FD=65535;
    
};
#endif