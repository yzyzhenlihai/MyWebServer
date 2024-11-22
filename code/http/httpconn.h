#ifndef _HTTPCONN_H_
#define _HTTPCONN_H_
#include<netinet/in.h>
#include<atomic>
#include<unistd.h>
#include"../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"
//记录每一个客户端连接
class HttpConn
{
private:
    int fd_;
    sockaddr_in addr_;
    Buffer ReadBuf_;
    Buffer WriteBuf_;
    bool isClose_;
    HttpRequest request_;
    HttpResponse response_;
    iovec iov_[2];
    size_t iovCnt;
public:
    HttpConn();
    ~HttpConn();
    void init(int fd,sockaddr_in addr);
    ssize_t RecvData(int* readErr);
    ssize_t SendData(int* sendErr);
    size_t ToWriteBytes();
    bool IsKeepAlive();
    bool Process();//处理请求报文以及构造响应报文
    int GetFd();
    void Close();
    bool isClose();
    static std::atomic<int> UserCnt;//记录连接的总数
    static bool isET;//ET模式需要一直读数据
    const static char* srcDir_;
};


#endif