#ifndef __LOG_HPP__
#define __LOG_HPP__


//该日志的作用是将错误的详细信息输出在终端上。
//内容包含：错误时间，错误等级，错误内容，错误所在文件名，错误所在行数
#include<iostream>
#include<string>
#include<sys/time.h>

//错误暂时分为4等
#define INFO 0
#define DEBUG 1
#define WARNING 2
#define ERROR 3

uint64_t GetTimeStamp()
{
    struct timeval time_;
    gettimeofday(&time_,NULL);
    return time_.tv_sec;
}


std::string GetLevelInfo(int level_)
{
    switch(level_)
    {
        case 0:
            return "INFO";
            break;
        case 1:
            return "DEBUG";
            break;

        case 2:
            return "WARNING";
            break;

        case 3:
            return "ERROR";
            break;

        default:
            return "UNKNOWN";
    }

}

void Log(int level_, std::string message_, std::string file_, int line_)    
{
    //2个参数分别是错误等级，错误信息
    //可以将日志写入文件，但为了更加直观（就是懒= =），我们就直接将日志打印出来吧
    //这里同时输出错误所在的文件名和行号
    std::cout<<"["<<GetTimeStamp()<<"] "<< "[" << GetLevelInfo(level_) << "]" << " "\
             <<"[" <<file_<<" :" <<line_<< "]"<<":"<< message_<<\
             std::endl;
}

#define Log(level_,message_)  Log(level_,message_,__FILE__,__LINE__)
#endif

