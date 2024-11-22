#include "log.h"

Log::Log(){}
Log::~Log(){}

Log* Log::getIntance(){
    static Log log_;
    return &log_;
}
//初始化Log类
void Log::Init(const char* path,const char* suffix,size_t blockSize){
    path_=path;
    suffix_=suffix;
    isAsyn_=false;
    logFd_=nullptr;
    isClose_=false;
    lineCnt_=0;
    buffer_.RetrieveAll();
    if(blockSize>0){
        //表示当前是异步日志
        isAsyn_=true;
        std::unique_ptr<BlockQueue<std::string>> newQue(new BlockQueue<std::string>);
        blockQue_=std::move(newQue);
        std::unique_ptr<std::thread> writeThread(new std::thread(logWriteThread));
        writeThread_=std::move(writeThread);
        writeThread_->detach();//设置分离，自动回收

    }
    //创建日志文件
    time_t timer=time(nullptr);//获得当前时间
    struct tm  *sysTime = localtime(&timer);
    char fileName[MAX_LOGNAME_LEN-1]={0};
    size_t year=sysTime->tm_year+1900;
    size_t month=sysTime->tm_mon+1;
    size_t day=sysTime->tm_mday;
    curYear_=year;
    curMon_=month;
    curDay_=day;
    snprintf(fileName,MAX_LOGNAME_LEN-1,"%s/%04ld_%02ld_%02ld%s",path_,year,month,day,suffix_);//构造文件路径
    //std::cout<<"Init fileName:"<<fileName<<std::endl;
    {
        std::unique_lock<std::mutex> locker(mutex_);
        buffer_.RetrieveAll();
        if(logFd_){
            //刷新缓冲区..
            fclose(logFd_);
        }
        logFd_=fopen(fileName,"a");
        if(logFd_==nullptr){
            mkdir(path_,0777);//创建./log文件夹
            std::cout<<"log fileName:"<<fileName<<std::endl;
            logFd_=fopen(fileName,"a");
        }
        assert(logFd_!=nullptr);
    }
}
//按格式写入日志文件
void Log::Write(int level,const char* format, ...){
    time_t timer=time(nullptr);
    struct tm *curTime=localtime(&timer);
    struct timeval now={0,0};
    gettimeofday(&now,nullptr);
    size_t year=curTime->tm_year+1900;//年
    size_t month=curTime->tm_mon+1;//月
    size_t day = curTime->tm_mday;//日
    //判断是否需要创建新日志文件
    if(curYear_!=year || curMon_!=month || curDay_!=day || (lineCnt_ && (lineCnt_% MAX_LINES)==0)){
        //创建新文件记录
        //std::unique_lock<std::mutex> locker(mutex_);
        char fileName[MAX_LOGNAME_LEN];
        if(curYear_!=year || curMon_!=month || curDay_!=day){
            snprintf(fileName,MAX_LOGNAME_LEN-1,"%s/%04ld_%02ld_%02ld%s",path_,year,month,day,suffix_);
            curYear_=year;
            curMon_=month;
            curDay_=day;
            lineCnt_=0;
        }else{
            snprintf(fileName,MAX_LOGNAME_LEN-1,"%s/%04ld_%02ld_%02ld-%ld%s",path_,year,month,day,(lineCnt_/MAX_LINES),suffix_);
        }
        std::unique_lock<std::mutex> locker(mutex_);
        fflush(logFd_);
        fclose(logFd_);
        logFd_=fopen(fileName,"a");
        assert(logFd_);
    }
    //开始构造日志内容
    {
        std::unique_lock<std::mutex> locker(mutex_);
        lineCnt_++;
        va_list valist;
        //写入记录时间
        int n=snprintf(buffer_.GetBeginWritePtr(),128,"%04d-%02d-%02d-%02d:%02d:%02d.%ld",
                        curTime->tm_year+1900,curTime->tm_mon+1,curTime->tm_mday,
                        curTime->tm_hour,curTime->tm_min,curTime->tm_sec,now.tv_usec);
        buffer_.HasWriten(n);
        writeLevelTag(level,buffer_);
        //写入自定义参数
        va_start(valist,format);
        int m=vsnprintf(buffer_.GetBeginWritePtr(),buffer_.GetWritableBytes(),format,valist);
        va_end(valist);
        buffer_.HasWriten(m);
        buffer_.Append("\n\0",2);
       
        if(isAsyn_){
            //异步日志
            //buffer_.ShowData();
            blockQue_->pushBack(buffer_.RetrieveAllToStr());
        }else{
            //直接写入日志文件
            fputs(buffer_.GetBeginReadPtr(),logFd_);
            buffer_.RetrieveAll();
        }
    }
    
}

void Log::Flush(){
    if(isAsyn_){
        //将阻塞队列中数据全部写入文件
        blockQue_->Flush();
    }
    fflush(logFd_);//将文件流缓冲区中的数据全部写入文件
}
//异步写线程
void Log::asynWrite(){
   
    std::string str="";
    while(blockQue_->popFront(str)){
        std::unique_lock<std::mutex> locker(mutex_);
        //std::cout<<"write Thread is excuting---str:"<<str<<std::endl;
        fputs(str.c_str(),logFd_);
        fflush(logFd_);
    }
}
void Log::logWriteThread(){
    Log::getIntance()->asynWrite();
}

bool Log::isOpen(){
    return isClose_==false;
}

void Log::writeLevelTag(int level,Buffer &buffer_){
    switch (level)
    {
    case 0:
        /* code */
        buffer_.Append("[DEBUG]:",8);
        break;
    case 1:
        buffer_.Append("[INFO]:",7);
        break;
    case 2:
        buffer_.Append("[ERROR]",7);
    default:
        break;
    }
}