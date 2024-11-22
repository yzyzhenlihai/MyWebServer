#include "webserver.h"
#include<sys/socket.h>
#include<netinet/ip.h>
WebServer::WebServer(int port,int maxevents,int timeoutMS,int threadNum,
                     int sqlPort,const char *sqlUserName,const char *sqlPwd,
                     const char *dbName,int connNum,bool isLog)
    :port_(port),epoll_(new Epoller(maxevents)),timeoutMS_(timeoutMS),threadPool_(new ThreadPool(threadNum)),timer_(new Timer()){
    //初始化连接池
    SqlConnPool::getIntance()->Init("localhost",sqlPort,sqlUserName,sqlPwd,dbName,connNum);
    srcDir_=getcwd(nullptr,256);//获得系统当前绝对路径
    strncat(srcDir_,"/resources/",16);
    HttpConn::srcDir_=srcDir_;
    //开启日志
    isClose=false;
    InitEventMode();
    if(!InitSocket()) isClose=true;
    
    if(isLog){
        Log::getIntance()->Init("./log",".log",10);
        if(isClose) { LOG_ERROR("========== Server init error!=========="); }
        else{
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("srcDir: %s", HttpConn::srcDir_);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connNum, threadNum);
        }
    }
    
    
    
}

WebServer::~WebServer(){
    close(listenFd_);
    isClose=true;
}
//初始化
bool WebServer::InitSocket(){
    listenFd_=socket(AF_INET,SOCK_STREAM,0);
    if(listenFd_<0) return false;
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port_);
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    char serverIP[1024];
    std::cout<<"server ip:"<<inet_ntop(AF_INET,&addr.sin_addr.s_addr,serverIP,sizeof(serverIP))<<std::endl;
    //一般在地址绑定前设置端口复用
    int opt=1;//设置端口复用
    int ret = setsockopt(listenFd_,SOL_SOCKET,SO_REUSEADDR,(void*)&opt,sizeof(opt));
    //LOG_DEBUG("REUSEADDR is set");
    assert(ret==0);
    if(bind(listenFd_,(struct sockaddr*)(&addr),sizeof(addr))==-1){
        std::cout<<"bind is fail"<<std::endl;
        return false;
    }
    //LOG_DEBUG("address bind successfully!");
    if(listen(listenFd_,128)==-1) return false;
    epoll_->AddFd(listenFd_,listenEvent_|EPOLLIN);
    //设置非阻塞模式
    SetFdNonblock(listenFd_);
    
    //std::cout<<"InitSocket is successful"<<std::endl;
    return true;
}

void WebServer::InitEventMode(){
    //都设置为ET模式
    listenEvent_=EPOLLET | EPOLLRDHUP;
    connEvent_=EPOLLET | EPOLLONESHOT | EPOLLHUP;//设置EPOLLNOESHOT，就不会存在一个socket同时被多个工作线程处理,但是每次处理完都要重新设置
}
//设置非阻塞模式
void WebServer::SetFdNonblock(int fd){
    int flag=fcntl(fd,F_GETFL);
    flag |=O_NONBLOCK;
    fcntl(fd,F_SETFL,flag);
}
//发送信息给指定fd
void WebServer::SendError(int fd,const char *err){
    write(fd,err,strlen(err));
}
//处理监听
void WebServer::DealListen(){
    sockaddr_in clientAddr;
    socklen_t clientAddrLen=sizeof(clientAddr);
    do{
        int clientfd=accept(listenFd_,(struct sockaddr*)(&clientAddr),&clientAddrLen);
        if(clientfd<=0){return;}
        else if(HttpConn::UserCnt>=MAX_FD){
            //连接数量超限，拒绝连接
            SendError(clientfd,"Too many connections, connection rejected\n");
            close(clientfd);
            return;
        }
        //打印客户端的ip和端口
        char clientIP[1024];
        inet_ntop(AF_INET,&clientAddr.sin_addr.s_addr,clientIP,sizeof(clientIP));
        epoll_->AddFd(clientfd,connEvent_ | EPOLLIN);
        SetFdNonblock(clientfd);
        //建立一条连接
        users[clientfd].init(clientfd,clientAddr);
        //添加超时定时器
        if(timeoutMS_>0){
            timer_->Add(clientfd,timeoutMS_,std::bind(&WebServer::CloseConn,this,&users[clientfd]));
        }
        LOG_INFO("Client[%d] is connected   ip:%s   port:%d    TotalCount:%d",
                clientfd,clientIP,ntohs(clientAddr.sin_port),HttpConn::UserCnt.load());
    }while(listenEvent_ & EPOLLET);
}
//处理读任务
void WebServer::DealRead(HttpConn *conn){
    //重置定时器时间
    ExtendTime(conn);
    //将任务添加至请求队列
    threadPool_->AddTask(std::bind(&WebServer::OnRead,this,conn));
    
}
void WebServer::OnRead(HttpConn *conn){
    assert(conn);
    int readErr;
    ssize_t len=conn->RecvData(&readErr);//读取数据
    if(len<=0 && readErr!=EAGAIN){
        conn->Close();
        return;
    }
    OnProcess(conn);//处理数据
    //std::cout<<"DealRead over"<<std::endl;
}

//延长指定文件描述符的定时器事件
void WebServer::ExtendTime(HttpConn *conn){
    assert(conn);
    if(timeoutMS_>0) timer_->justTimer(conn->GetFd(),timeoutMS_);
}
//处理写任务
void WebServer::DealWrite(HttpConn *conn){
    //重置定时器时间
    ExtendTime(conn);
    //将任务添加至请求队列
    assert(conn);
    threadPool_->AddTask(std::bind(&WebServer::OnWrite,this,conn));
}

void WebServer::OnWrite(HttpConn *conn){
    assert(conn);
    int writeErr;
    ssize_t len=conn->SendData(&writeErr);
    if(conn->ToWriteBytes()==0){
        //传输完成
        if(conn->IsKeepAlive()){
            OnProcess(conn);
        }
    }else if(len<0){
        if(writeErr==EAGAIN){
            //继续传输
            epoll_->ModFd(conn->GetFd(),connEvent_ | EPOLLOUT);
        }
    }

}

//修改客户端连接的监听事件
void WebServer::OnProcess(HttpConn *conn){
    assert(conn);
    if((conn->Process())){
        //重新挂上监听红黑树
        int fd=conn->GetFd();
        epoll_->ModFd(fd,connEvent_ | EPOLLOUT);
    }else{
        int fd=conn->GetFd();
        epoll_->ModFd(fd,connEvent_ | EPOLLIN);
    }
}
//关闭连接
void WebServer::CloseConn(HttpConn *conn){
    assert(conn);
    if(!conn->isClose()){
        LOG_INFO("client[%d] is closed",conn->GetFd());
         //取消监听
        epoll_->DelFd(conn->GetFd());
        //删除连接
        conn->Close();
    }
   
}

void WebServer::Start(){
    //开始监听
    while(!isClose){
        int timeout=timer_->getNextTick();
        int requestNum = epoll_->Wait(timeout);
        for(int i=0;i<requestNum;i++){
            int fd=epoll_->GetEventFd(i);
            uint32_t event=epoll_->GetEvent(i);
            if(fd==listenFd_){
                //处理监听
                DealListen();
            }else{
                if(event & EPOLLIN){
                    //处理读
                   // std::cout<<"DealRead..."<<std::endl;
                    assert(users.count(fd) > 0);
                    DealRead(&users[fd]);
                }else if(event & EPOLLOUT){
                    //处理写
                    //std::cout<<"DealWrite..."<<std::endl;
                    assert(users.count(fd) > 0);
                    DealWrite(&users[fd]);
                }else if(event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                    //关闭连接
                    assert(users.count(fd) > 0);
                    CloseConn(&users[fd]);
                }
            }
        }
    }
}
