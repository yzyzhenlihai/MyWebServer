#ifndef LOG_H_
#define LOG_H_
#include<mutex>
#include<ctime>
#include<sys/stat.h>
#include<sys/time.h>
#include<stdarg.h>
#include<thread>
#include "../buffer/buffer.h"
#include "blockqueue.h"
//单例模式
class Log{
public:
    
    void Init(const char* path="./log",const char* suffix=".log",size_t blockSize=1024);//初始化Log类
    void Write(int level,const char* format, ...);//写日志文件
    static void logWriteThread();
    static Log* getIntance();
    bool isOpen();
    void Flush();//强行刷新缓冲区
    void writeLevelTag(int level,Buffer &buffer_);

private:
    Log();
    ~Log();
    std::unique_ptr<BlockQueue<std::string>> blockQue_;//阻塞队列
    std::unique_ptr<std::thread> writeThread_;//异步写线程
    std::mutex mutex_;
    const char* path_;//所在路径
    const char* suffix_;//后缀名
    FILE *logFd_;//日志文件
    bool isAsyn_;//是否异步
    Buffer buffer_;//暂存缓冲区
    size_t lineCnt_;//当前行数
    size_t curYear_;//当前年份
    size_t curMon_;//当前月份
    size_t curDay_;//记录当前日期

    const static size_t MAX_LOGNAME_LEN=512;//最大文件名长度
    const static size_t MAX_LINES=50000;//最多行数

    bool isClose_;//是否被关闭
private:
    
    void asynWrite();//异步写
   
};
//写入日志
#define LOG_BASE(level,format,...) \
    do{\
        Log* log=Log::getIntance();\
        if(log->isOpen()){\
            log->Write(level,format,##__VA_ARGS__);\
            log->Flush();\
        }\
    }while(0);

#define LOG_DEBUG(format,...) do{LOG_BASE(0,format,##__VA_ARGS__)}while(0);
#define LOG_INFO(format,...) do{LOG_BASE(1,format,##__VA_ARGS__)}while(0);
#define LOG_ERROR(format,...) do{LOG_BASE(2,format,##__VA_ARGS__)}while(0);
#endif