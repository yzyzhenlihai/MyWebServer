#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_
#include<string>
#include "../buffer/buffer.h"
#include<algorithm>
#include<iostream>
#include<regex>
#include<unordered_map>
#include<unordered_set>
#include<sstream>
#include<mysql/mysql.h>
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"
#include "../log/log.h"

class HttpRequest{
public:
    //当前解析状态
    enum PARSE_STATE{
        REQUEST_LINE,//请求行
        REQUEST_HEADER,//请求头
        REQUEST_BODY,//请求体
        FINISH,
    };
    //请求处理结果
    enum HTTP_CODE{
        NO_REQUEST,//请求不完整
        BAD_REQUEST,//请求有语法错误
        GET_REQUEST,//获得请求
    };
    HttpRequest();
    ~HttpRequest();
    void Init();
    bool parse(Buffer &buffer_);
    bool IsKeepAlive();
    std::string GetPath_();
private:
    PARSE_STATE state_;//请求解析状态
    std::string method_;//请求方法
    std::string path_;//资源路径
    std::string version_;//http版本
    std::string body_;//请求体
    std::unordered_map<std::string,std::string> header_;//请求头
    std::unordered_map<std::string,std::string> post_;//请求体键值对
    bool ParseRequestLine_(const std::string &line);
    void ParseRequestHeader_(const std::string &line);
    void ParseRequestBody_(const std::string &line);
    void ParsePost_();
    void ParseFromUrlencoded_();
    void ParsePath_();
    
    bool DealLogin();//处理登录
    bool DealRegister();//处理注册
    static const std::unordered_set<std::string> DEFAULT_HTML;
    void ShowParseResult();//展示请求报文解析结果
};

#endif