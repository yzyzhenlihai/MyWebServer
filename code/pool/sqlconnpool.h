#ifndef _SQLCONNPOOL_H_
#define _SQLCONNPOOL_H_
#include<mysql/mysql.h>
#include<queue>
#include<iostream>
#include<mutex>
#include<semaphore.h>
#include<assert.h>
//单例模式
class SqlConnPool{
public:
    static SqlConnPool* getIntance();
    void Init(const char* host,const unsigned int port,
            const char* userName,const char *pwd,
            const char* dbName,int connSize);
    MYSQL* GetConn();//获得连接
    void BackConn(MYSQL*);//归还连接
    void CloseSqlConnPool();//关闭连接池
private:
    SqlConnPool();
    ~SqlConnPool();
    std::queue<MYSQL*> connQueue;//连接池队列
    std::mutex mutex_;//互斥锁
    sem_t semConn_;//信号量
    int MAX_CONN;//记录最大连接数
    static SqlConnPool* connPool_;
};


#endif