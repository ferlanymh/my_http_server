//protocol_util(协议工具)内含：
//1.处理服务端请求Request
//2.给服务端响应Response的所有详细代码
//
#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__

#pragma GCC diagnostic error "-std=c++11" 
#include<iostream>
#include<string>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<strings.h>
#include<sstream>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<unordered_map>
#include"Log.hpp"
#define WEB_ROOT "mywwwroot"
#define HOME_PAGE "index.html"
#define PAGE_404 "404.html"
#define OK 200
#define NOT_FOUND 404 
#define BAD_REQUEST 400
#define SERVER_ERROR 500


#define HTTP_VERSION "HTTP/1.0"
using namespace std;

unordered_map<string,string> suffix_map 
{
  {".html","text/html"},
    {".htm","text/html"},
    {".css","text/css"},
    {".js","application/x-javascript"},
    {".jpg", "image/jpeg"},
    {".png", "image/png"},
};


class StringUtil{
  public:
    static string SuffixToType(const string& suffix_)
    {
      return suffix_map[suffix_]; 
    }
    static string IntToString(int code)
    {
      std::stringstream ss;
      ss <<code;
      return ss.str();
    }
    static string CodeToDesc(int code)
    {
      switch(code)
      {
        case 200:
          return "OK";

        case 400:
          return "BAD_REQUEST";

        case 404:
          return "NOT FOUND";

        case 500:
          return "Internal Serval Error";
        default:
          return "Unknow";
      }
    }

    static void MakeKV(std::unordered_map<std::string, std::string> &kv_,\
        std::string &str_)
    {
      std::size_t pos = str_.find(": ");
      if(std::string::npos == pos){
        return;
      }

      std::string k_ = str_.substr(0, pos);
      std::string v_ = str_.substr(pos+2);

      kv_.insert(make_pair(k_, v_));
    }
};



class Request//存储http请求的各项数据信息
{
  public:
    std::string rq_line;//首行：(请求方法/url/版本信息)
    std::string rq_head;//报头(内部都是kv键值对,键值对间以"&"分隔，kv以": "分隔，有多行)
    std::string blank;//空行(不论是http请求还是响应都不可以省略)
    std::string rq_text;//正文
  private:
    std::string method;//请求方法
    std::string uri;//请求资源路径（可能含参数）
    std::string version;//协议版本

    bool cgi;//使用cgi的情况是method=="POST"  或者  method=="GET"且uri中有参数
    std::string path;//path中存储资源路径
    std::string param;//param中存储资源参数

    int resource_size;//资源大小
    unordered_map<string,string> head_kv;//报头解析后放置的容器里
    int content_length;//正文长度
    std::string suffix;//请求资源的后缀名


  public:
    Request():blank("\n"),cgi(false),path(WEB_ROOT),resource_size(0),content_length(-1),suffix(".html")
  {}

    std::string& GetPath()
    {
      return path;
    }
    void SetPath(string path_)
    {
      path = path_;
    }
    std::string& GetSuffix()
    {
      return suffix;
    }
    std::string& SetSuffix(string suffix_)
    {
      suffix = suffix_;
    }
    bool IsCgi()
    {
      return cgi;
    }
    int GetResource_size()
    {
      return resource_size;
    }
    void SetResource_size(int res_size)
    {
      resource_size = res_size;
    }

    int GetContentLength()
    {
      string cont_len = head_kv["Content-Length"];
      if (cont_len.empty())

      {
        return 0;  
      }
      stringstream ss(cont_len);
      ss >> content_length;
      return content_length;
    }
    bool IsNeedReadText()
    {
      if (strcasecmp(method.c_str(),"POST") == 0)
      {
        return true;
      }
      return false;
    }

    void RequestLineParse()//对请求首行进行解析:例如   GET /A/B/C.html/http /1.1
    {
      //首行：请求方法,uri，版本信息3部分
      stringstream ss(rq_line);
      ss >> method >> uri >> version;
    }

    void UriParse()//解析uri，设置path与param
    {
      if (strcasecmp(method.c_str(),"GET") == 0)//如果是get方法
      {

        size_t pos_ = uri.find('?');
        if (string::npos !=pos_)
        {
          //说明在uri中发现了字符'?'，那么显而易见的这个方法是GET且'?'后面是资源参数
          //'?'前面的是path，'?'后面的是param
          cgi=true;
          path += uri.substr(0 , pos_);//

          param = uri.substr(pos_ + 1);
        }
        else//说明uri中不含参数，直接全粘在path后面即可
        {
          path += uri;
        }
      }
      else//说明是POST方法
      {
        path+=uri;
      }
      if (path[path.size()-1]=='/')
      {
        //当path以'/'结尾，说明path指的是根目录，我们需要给他返回一个默认首页。
        path += HOME_PAGE;
      }
    }

    bool RequestHeadParse()
    {
      //请求报头之前已经放置在rq_head中了，直接拆字串放进unordered_map里

      int start = 0;
      while ((size_t)start<rq_head.size())
      {
        size_t pos = rq_head.find('\n' , start);
        if (pos == string::npos)
        {

          break;
        }


        string sub_string_ = rq_head.substr(start,pos-start);//找到了以'\n'分隔的各个子串
        if (!sub_string_.empty())
        {
          //在这里又要把字串拆分成一个个的键值对并插入head_kv中
          Log(INFO,"request head parse is not empty!!");
          StringUtil::MakeKV(head_kv,sub_string_);
        }
        else
        {
          Log(INFO,"the head of request is empty!!");

        }    

        start = pos + 1;
      }
      return true;
    }

    bool CheckMethodLegal()//检查method
    {
      if ((strcasecmp(method.c_str(),"GET") ==0 ) ||\
          (cgi= (strcasecmp(method.c_str(),"POST") ==0)) )

      {
        Log(INFO,"method is legal!!");
        
        return true;
      }
      else
        return false;
    }


    bool CheckPathLegal()//检查资源路径的合法性
    {
      //这里使用stat函数，判断Path路径下是否有我们想要的资源。
      struct stat s;
      if (stat(path.c_str(),&s) < 0  )
      {
        //说明该路径非法
        Log(WARNING,"The path is illegal!!");
        return false;
      }


      //这里仍需要判断一下该资源是否是目录或者是可执行文件
      if (S_ISDIR(s.st_mode))
      {
        //是目录文件我们就返回默认首页给用户
        //目录最后结尾并不‘/’(因为我们在uri解析时已经判断过了)
        //比如192.168.xx.xx/a，这里就不能直接给后面补HOME_PAGE
        path += '/';
        path += HOME_PAGE;
      }
      else//这里判断是否是可执行文件，stat结构里有权限的标识符
      {
        if ((s.st_mode & S_IXUSR) ||\
            (s.st_mode & S_IXGRP) ||\
            (s.st_mode & S_IXOTH))
        {
          cgi=true ;
        }
      }

      resource_size=s.st_size;
      std::size_t pos =path.rfind(".");
      if (string::npos != pos)
      {
        suffix = path.substr(pos);
      }
      return true;
    }


    string& getline()
    {
      return rq_line;
    }
    string& GetParam()
    {
      return param;
    }
    ~Request()
    {}
};

class Response//存储http响应的各项数据信息
{
  private:
    std::string rsp_line;//首行(版本信息/状态码/状态码描述)
    std::string rsp_head;//报头(内部是kv键值对，键值对间以"&"分隔，kv以": "分隔,有多行)
    std::string blank;//空行
    std::string rsp_text;//正文

    int code;//状态码
    int fd;//打开资源的文件描述符
  public:
    Response():blank("\n"),code(OK),fd(-1)
  {}
    string& GetLine()
    {
      return rsp_line;
    }
    string& GetText()
    {
      return rsp_text;
    }

    string& GetHead()
    {
      return rsp_head;
    }

    string& GetBlank()
    {
      return blank;
    }

    int& getcode()
    {
      return code;
    }
    int GetFd()
    {
      return fd;
    }
    void OpenResource(Request* &rq_)
    {
      string path_ = rq_->GetPath();
      fd = open(path_.c_str(),O_RDONLY);
    }
    void MakeResponseLine()
    {
      rsp_line = HTTP_VERSION;
      rsp_line += " ";
      rsp_line +=StringUtil::IntToString(code);
      rsp_line += " ";
      rsp_line +=StringUtil::CodeToDesc(code);
      rsp_line +="\n";
    }

    void MakeResponseHead(Request* &rq_)
    {
      rsp_head = "Content-Length: ";
      rsp_head += StringUtil::IntToString(rq_->GetResource_size());
      rsp_head += "\n";

      rsp_head += "Content-Type: ";
      std::string suffix_ = rq_->GetSuffix();
      rsp_head += StringUtil::SuffixToType(suffix_); 
      rsp_head += "\n";
    }

    ~Response()
    {
      if (fd>=0)
      {
        close(fd);
      }
    }
};


class Connect//这个结构体专门提供读取http请求与设计http响应并返回资源的函数接口。
{
  private:
    int sock;
  public:
    Connect(int sock_):sock(sock_)
  {}

    void RecvRequestHead(std::string &head_)
    {
      head_ = "";
      std::string line_;
      while(line_ != "\n"){
        line_ = "";
        RecvOneLine(line_);
        head_ += line_;
      }
    }
    int RecvOneLine(string& str)//一个一个字符读，直到读完一行
    {
      //这里要做一个说明：因为浏览器(服务端)的版本不同，他们对于读完一行的标志不同。
      //有的认为读到"\n"算读完一行，有的认为读到"\n\r"算读完一行，甚至有的认为读到"\r"就算读完一行。
      //那么如果不统一，那么后期的对请求和响应的处理会很冗杂，故在此同一转换为"\n"算读完一行。
      //
      char c='J';//随便给c赋个值，只要不是"\n"就行
      while (c!='\n')
      {
        ssize_t s = recv( sock , &c , 1 ,0);
        //这里要注意一个坑！根据上面的分析，我们读取到"\r"后，仍不能确定是否已经读完一行，因为后面
        //可能还有1个"\n"。但是如果贸然再读一个字符，可能会把下一行的数据给读走，这就拿走了本不应该
        //读取的数据。
        //所以这里我们要引入一个参数MSG_PEEK(窥探)，这个值可以作为recv的第四个参数，他的功能
        //是读取数据的拷贝，但并不从缓冲区中拿走这个数据。引用这个参数我们就能轻松解决这个问题了~
        if (s > 0)
        {
          if (c=='\r')
          {
            recv( sock ,&c ,1 ,MSG_PEEK );
            if (c=='\n')
            {
              //运行到这里说明读到的是"\r\n"，这时候就要把这个字符从缓冲区中真正的读出来。然后算作一行读完
              recv(sock,&c,1,0);
            }

            else
            {
              //运行到这里说明窥探的字符并不是"\n",所以"\r"就是一行的结尾。
              c='\n';
            }

          }
          str.push_back(c);
        }
        else
        {
          break;
        }
      }
      
      return str.size();
    }

    void ReadRqHead(string& rq_head)
    {
      //报头可能有很多行，所以我们只要循环调用RecvOneLine即可。
      string line_;
      rq_head = "";
      while(strcmp(line_.c_str(),"\n"))//报头以'\n'为结尾，读到最后一行时，line_内的数据就是"\n"
      {
        line_ = "";//这里要注意每次进入循环时要将line_置为空，因为每行读的数据都还在line_里，要注意！
        RecvOneLine(line_);
        rq_head += line_;
      }

    }
    void RecvRequestText(string &text_,int len,string& param)
    {
      char c;
      int i=0;
      while (i<len)
      {
        recv(sock,&c , 1 , 0);
        text_.push_back(c);
        i++;
      }
      param = text_;
    }

    void SendResponse(Response* &rsp_,Request* &rq_)
    {
      //发送数据
      string &rsp_line = rsp_->GetLine();
      string &rsp_head = rsp_->GetHead();
      string &blank = rsp_->GetBlank();
      string &rsp_text = rsp_->GetText();
      send(sock,rsp_line.c_str(),rsp_line.size(),0);
      send(sock,rsp_head.c_str(),rsp_head.size(),0);
      send(sock,blank.c_str(),blank.size(),0);
      if (rq_->IsCgi())
      {
        send(sock,rsp_text.c_str(),rsp_text.size(),0);
      }
      else
      {
        sendfile(sock,rsp_->GetFd(),NULL,rq_->GetResource_size());
      }


    }

    ~Connect()
    {
      if (sock > 0)
      {
        close(sock);
      }
    }
};

class Entry//内部只有RequestHandler入口函数，RequestHandler函数是统领全局的函数，调用其他类的接口完成读取请求并返回响应。
{
  public:
    static void ProcessCgi(Connect* &conn_,Request* &rq_,Response* &rsp_)
    {
      int& code_ = rsp_->getcode();
      //cgi方式处理
      int input[2];//父进程将参数送至子进程的匿名管道
      int output[2];//子进程处理完参数送回父进程的匿名管道
      pipe(input);
      pipe(output);


      pid_t pid = fork();
      if (pid < 0 )
      {
        code_ = SERVER_ERROR;
        Log(ERROR,"fork process is failed!!!");
        return ;
      }

      if (pid == 0)//child
      {
        close(input[1]);
        close(output[0]);


        std::string param_ = rq_->GetParam();
        std::string cl_env_ = "Content-Length=";
        cl_env_ += StringUtil::IntToString(param_.size());
        putenv((char*)cl_env_.c_str());//给当前进程的上下文中添加自定义环境变量。

        //当子进程执行exec函数时，无法找到匿名管道对应的文件描述符，
        //所以我们重定向至0，1
        dup2(input[0] ,0);
        dup2(output[1] ,1);

        string path_ = rq_->GetPath();
        execl(path_.c_str(),path_.c_str(),nullptr);

        //运行到此处说明exec调用失败
        Log(ERROR,"use execl function is failed!!");
        exit(1);
      }

      else//parent
      {
        close(input[0]);
        close(output[1]);

        string param_ = rq_->GetParam();
        const char *ptr = param_.c_str();
        size_t total = param_.size();//要给子进程发送的数据长度
        size_t already_send = 0;//当前已经发送的长度
        size_t size = 0;//本次发送的长度

        while (total > already_send)
        {
          size = write(input[1], ptr + already_send  ,total - already_send );
          if (size)
          {
            already_send += size;
          }
          else
          {
            break;
          }

        }

        char c;
        while ( (size=read(output[0], &c ,1 ) > 0 ) )
        {
          rsp_->GetText().push_back(c);
        }

        waitpid(pid,NULL,0);
        close(input[1]);
        close(output[0]);

        rsp_->MakeResponseLine();
        rq_->SetResource_size(rsp_->GetText().size());//注意响应报头中content-length可能改变，要重新设置      
        rsp_->MakeResponseHead(rq_);
        conn_->SendResponse(rsp_,rq_);
      }   
    }
    static void ProcessNoneCgi(Connect* &conn_,Request* &rq_,Response* &rsp_)
    {
      rsp_->MakeResponseLine();//响应首行
      rsp_->MakeResponseHead(rq_);//响应报头
      rsp_->OpenResource(rq_);//将资源打开，用fd描述之
      conn_->SendResponse(rsp_ , rq_);//发送响应


    }
    static void MakeResponse(Connect* &conn_,Request* &rq_,Response* &rsp_)
    {
      if (rq_->IsCgi())
      {
        Log(INFO,"The request needs cgi response!!");
        ProcessCgi(conn_,rq_,rsp_);
      }
      else 
      {
        Log(INFO,"The request does not need cgi response!!");
        ProcessNoneCgi(conn_,rq_,rsp_);
      }
    }

    static void Process404( Request *&rq_,Response* &rsp_,Connect* &conn_)
    {
      std::string path_ = WEB_ROOT;
      path_ += "/";
      path_ += PAGE_404;
      struct stat st;
      stat(path_.c_str(), &st);

      rq_->SetResource_size(st.st_size);
      rq_->SetSuffix(".html");
      rq_->SetPath(path_);

      ProcessNoneCgi(conn_,rq_,rsp_);
    }
    static void Handler_Error( Request *&rq_,Response* &rsp_,Connect* &conn_)
    {
      int& code = rsp_->getcode();
      switch(code)
      {
        case 400:
          break;

        case 404:
          Process404(rq_,rsp_,conn_);
          break;

        case 500:
          break;

        case 503:
          break;
      }
    }
    static int  RequestHandler(int sock_)
    {
      //因为要实现读取请求并返回响应，故创建connect,response,request对象，分别调用他们的接口。
      Connect *conn_ = new Connect(sock_);
      Request *rq_ = new Request();
      Response *rsp_ = new Response();

      //接下来的步骤就是读取http请求，先对请求中的首行进行解析
      conn_->RecvOneLine(rq_->rq_line);
      Log(INFO,"get rq_line!!");
      cout<<rq_->getline()<<endl;
      rq_->RequestLineParse();//旨在将rq_line的内容分别放在3个字段中
      Log(INFO,"rq_line parse done!!");
      int& code_ = rsp_->getcode();//获取状态吗

      if (!(rq_->CheckMethodLegal()) )//判断方法的合法性，顺便判断后面处理数据是否需要使用cgi
      {
        conn_->ReadRqHead(rq_->rq_head);//退出前要读完请求！
        Log(ERROR,"request method is illegal");
        code_ = BAD_REQUEST;
        goto end;
      }

      //运行到这里说明方法已经合法，现在进行uri的解析。
      rq_->UriParse();
      Log(INFO,"uri parse is done!!");
      if (!rq_->CheckPathLegal( ))
      {
        conn_->ReadRqHead(rq_->rq_head);
        code_ = NOT_FOUND;
        goto end; 
      }

      Log(INFO,"the path of resource is ok!!");

      //版本这里就不作解读了
      //下面开始读取并解析http请求的第二部分：报头

      conn_->ReadRqHead(rq_->rq_head);//读取报头并发在rq_->head字段内
      if (rq_->RequestHeadParse())
      {
        Log(INFO,"parse request head is done!!");
      }
      else
      {

        Log(ERROR,"request head is illegal");
        code_=BAD_REQUEST;
        goto end;
      }
       
      //报头报读完毕后，要检查是否有正文存在
      if (rq_->IsNeedReadText())
      {
        //读取正文
        conn_->RecvRequestText(rq_->rq_text,rq_->GetContentLength(),rq_->GetParam());
        Log(INFO,"rq_text is done!");
      }

      //到此读完请求
      Log(INFO,"handler request is done!!!");
     
      
      MakeResponse(conn_,rq_,rsp_);//开始发送响应
      Log(INFO,"response is already send!!");

      //到这里响应发出，程序结束！

end:
      if (code_!=OK)
      {
        Handler_Error(rq_,rsp_,conn_);
      }

      delete rsp_;
      delete rq_;
      delete conn_;
      close(sock_);
    }


};

#endif
