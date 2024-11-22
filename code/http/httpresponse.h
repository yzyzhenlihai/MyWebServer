#ifndef HTTPRESPONSE_H_
#define HTTPRESPONSE_H_
#include "../buffer/buffer.h"
#include<string>
#include<sys/stat.h>
#include<unordered_map>
#include<sys/mman.h>
#include<fcntl.h>
class HttpResponse{
public:
    HttpResponse();
    ~HttpResponse();
    void Init(const std::string srcDir,std::string path,bool isKeepAlive,int code);
    void MakeResponse(Buffer &buffer_);
    char* GetFile();
    size_t GetFileLen();
    void ShowResponseResult(Buffer &buffer_);//展示响应结果
private:

    int code_;//状态码
    std::string path_;//资源路径
    std::string srcDir_;//完成路径
    struct stat statBuf_;//文件相关属性
    char* mmFile_;//共享内存映射区
    bool isKeepAlive_;
    const static std::unordered_map<int,std::string> CODE_PATH;
    const static std::unordered_map<int,std::string> CODE_STATE;
    const static std::unordered_map<std::string,std::string> SUFFIX_TYPE;
    void ErrorHtml_();
    void AddStateLine_(Buffer &buffer_);
    void AddHeader_(Buffer &buffer_);
    std::string GetFileType_();//获得响应文件的类型
    void AddContent(Buffer &buffer_);
    void UnmapFile_();//释放共享内存缓冲区
    
};


#endif