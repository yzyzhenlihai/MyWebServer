#include "sqlconnpool.h"
SqlConnPool* SqlConnPool::connPool_=nullptr;//静态变量还需要在实现类中定义一下
SqlConnPool::SqlConnPool(){}
SqlConnPool::~SqlConnPool(){
    CloseSqlConnPool();
}

//获得全局唯一实例
SqlConnPool* SqlConnPool::getIntance(){
    if(connPool_==nullptr){
        //双重检查锁，确保线程安全
        //std::unique_lock<std::mutex> locker(mutex_);
        //if(connPool_==nullptr){
            connPool_=new SqlConnPool();
       //}
    }
    return connPool_;
}
//获取连接
MYSQL* SqlConnPool::GetConn(){
    MYSQL* conn=nullptr;
    if(connQueue.empty()){
        std::cout<<"connQueue is busy"<<std::endl;
        return nullptr;
    }
    sem_wait(&semConn_);
    {
        std::unique_lock<std::mutex> locker(mutex_);
        conn=connQueue.front();
        connQueue.pop();
    }
    return conn;
}
//归还连接
void SqlConnPool::BackConn(MYSQL *conn){
    assert(conn);
    std::unique_lock<std::mutex> locker(mutex_);
    connQueue.push(conn);
    sem_post(&semConn_);
}
//关闭连接池
void SqlConnPool::CloseSqlConnPool(){
    MYSQL* conn;
    std::unique_lock<std::mutex> locker(mutex_);
    while(connQueue.size()){
        conn=connQueue.front();
        connQueue.pop();
        mysql_close(conn);
    }
    delete connPool_;//关闭连接池
}
//初始化sql连接池
void SqlConnPool::Init(const char* host,const unsigned int port,
        const char* userName,const char *pwd,const char* dbName,
        int connSize=10){
    assert(connSize>0);
    //创建连接
    for(int i=0;i<connSize;i++){
        MYSQL *sql=nullptr;
        sql=mysql_init(sql);//初始化sql连接
        if(!sql){
            std::cout<<"mysql init error"<<std::endl;
            assert(sql);
        }
        //std::cout<<"mysql init  successfully"<<std::endl;
        //与数据库建立连接
        sql=mysql_real_connect(sql,host,userName,pwd,dbName,port,nullptr,0);
        if(!sql){
            std::cout<<"mysql_real_connect error"<<std::endl;
            assert(sql);
        }
        //std::cout<<"mysql_real_connect successfully"<<std::endl;
        //加入连接队列
        connQueue.push(sql);
    }
    std::cout<<"sql pool is created"<<std::endl;
    MAX_CONN=connSize;
    //初始化信号量
    sem_init(&semConn_,0,MAX_CONN);
}