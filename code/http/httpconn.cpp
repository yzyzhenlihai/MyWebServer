#include "httpconn.h"

bool HttpConn::isET=true;//默认为ET模式
const char* HttpConn::srcDir_;
HttpConn::HttpConn(){
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
}
HttpConn::~HttpConn(){}
std::atomic<int> HttpConn::UserCnt;
void HttpConn::init(int fd,sockaddr_in addr){
    fd_=fd;
    addr_=addr;
    UserCnt.fetch_add(1);//总数加1
    ReadBuf_.RetrieveAll();
    WriteBuf_.RetrieveAll();
    iov_[0]=iov_[1]={0};
    iovCnt=0;
    isClose_=false;
}
//读取客户端请求
ssize_t HttpConn::RecvData(int* readErr){
    ssize_t len=0;
    while(isET){
        ssize_t len=ReadBuf_.ReadFd(fd_,readErr);
        if(len<=0){
            break;
        }
    }
    //ReadBuf_.ShowData();
    return len;
}
//向客户端发送响应报文
ssize_t HttpConn::SendData(int* sendErr){
    ssize_t len=0;
    while(isET){
        len=writev(fd_,iov_,2);
        if(len<=0){
            //错误处理
            *sendErr=errno;
            break;
        }
        if(iov_[0].iov_len+iov_[1].iov_len==0)break;//表示传输结束
        if(len>iov_[0].iov_len){
            iov_[1].iov_base=(uint8_t*)iov_[1].iov_base+(len-iov_[0].iov_len);
            iov_[1].iov_len-=(len-iov_[0].iov_len);
            if(iov_[0].iov_len){
                iov_[0].iov_len=0;
                WriteBuf_.RetrieveAll();
            }
        }else{
            iov_[0].iov_base=(uint8_t*)iov_[0].iov_base+len;
            iov_[0].iov_len-=len;
            WriteBuf_.Retrieve(len);
        }
    }
    return len;
}
//解析请求报文以及构造响应报文
bool HttpConn::Process(){
    request_.Init();
    if(ReadBuf_.GetReadableBytes()<=0) return false;//当前没有可以解析的请求
    if(request_.parse(ReadBuf_)){
        //构造响应报文...
        response_.Init(srcDir_,request_.GetPath_(),request_.IsKeepAlive(),200);
        response_.MakeResponse(WriteBuf_);
    }else return false;
    //将构造的响应报文写入iov中
    //response_.ShowResponseResult(WriteBuf_);
    iov_[0].iov_base=WriteBuf_.GetBeginReadPtr();
    iov_[0].iov_len=WriteBuf_.GetReadableBytes();
    iovCnt=1;
    if(response_.GetFile() && response_.GetFileLen()>0){
        iov_[1].iov_base=response_.GetFile();
        iov_[1].iov_len=response_.GetFileLen();
        iovCnt=2;
    }
    
    return true;
}
int HttpConn::GetFd(){
    return fd_;
}
void HttpConn::Close(){
    
    if(isClose_==false){
        close(fd_);
        UserCnt.fetch_sub(1);
        isClose_=true;
    }
    
}
//计算还未发送的数据
size_t HttpConn::ToWriteBytes(){
    return iov_[0].iov_len+iov_[1].iov_len;
}
bool HttpConn::IsKeepAlive(){
    return request_.IsKeepAlive();
}
bool HttpConn::isClose(){
    return isClose_==true;
}
