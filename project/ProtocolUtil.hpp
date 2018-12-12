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
#define OK 200
#define NOT_FOUND 404 
#define HTTP_VERSION "HTTP/1.0"
using namespace std;

unordered_map<string,string> suffix_map 
{
    {".html","text/html"},
    {".htm","text/html"},
    {".css","text/css"},
    {".js","application/x-javascript"}
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

        case 404:
          return "NOT FOUND";
          
        default:
        return "UNKNOW";
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
    Request():blank("\n"),cgi(false),path(WEB_ROOT),resource_size(0),content_length(-1)
  {}

    std::string& GetPath()
    {
      return path;
    }
    std::string& GetSuffix()
    {
      return suffix;
    }
    bool IsCgi()
    {
      return cgi;
    }
    int GetResource_size()
    {
      return resource_size;
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
      if (strcasecmp(method.c_str(),"POST"))
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
      if (strcmp(method.c_str(),"GET"))//如果是get方法
      {
        
        size_t pos_ = uri.find('?');
        if (pos_ != string::npos)
        {
          //说明在uri中发现了字符'?'，那么显而易见的这个方法是GET且'?'后面是资源参数
          //'?'前面的是path，'?'后面的是param
          cgi=true;
          path += uri.substr(pos_);//
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
        path += WEB_ROOT;
      }
    }

    bool RequestHeadParse()
    {
      //请求报头之前已经放置在rq_head中了，直接拆字串放进unordered_map里
      string sub_string_ = "";
      size_t start = 0;
      while (start < rq_head.size())
      {
        size_t pos = rq_head.find('\n' , start);
        if (pos == string::npos)
        {
          break;
        }
        else
        {
          sub_string_ = rq_head.substr(start,pos-start);//找到了以'\n'分隔的各个子串
          start = pos + 1;
          if (sub_string_.empty())
          {
            //在这里又要把字串拆分成一个个的键值对并插入head_kv中
            StringUtil::MakeKV(head_kv,sub_string_);
          }
          else
            break;
        }
      }
      return true;
    }

    bool CheckMethodLegal()//检查method
    {
      cgi=strcasecmp(method.c_str(),"POST");
      if (strcasecmp(method.c_str(),"GET") || cgi)
      {
        return true;
      }
      else
        return false;
    }


    bool CheckPathLegal()//检查资源路径的合法性
    {
      //这里使用stat函数，判断Path路径下是否有我们想要的资源。
      struct stat s;
      if (!stat(path.c_str(),&s) )
      {
        //说明该路径非法
        Log(WARNING,"the path is illegal!!");
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
          cgi = true;
        }
      }

      resource_size=s.st_size;
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
	Connect(int sock_) :sock(sock_)
	{}

	void RecvRequestHead(std::string &head_)
	{
		head_ = "";
		std::string line_;
		while (line_ != "\n") {
			line_ = "";
			RecvOneLine(line_);
			head_ += line_;
		}
	}
	int RecvOneLine(string& str)//一个一个字符读，直到读完一行
	{
		
		char c = 'J';//随便给c赋个值，只要不是"\n"就行
		while (c != '\n')
		{
			ssize_t s = recv(sock, &c, 1, 0);
			//根据上面的分析，我们读取到"\r"后，仍不能确定是否已经读完一行，因为后面
			//可能还有1个"\n"。但是如果贸然再读一个字符，可能会把下一行的数据给读走，这就拿走了本不应该
			//读取的数据。
			
			if (s > 0)
			{
				if (c == '\r')
				{
					recv(sock, &c, 1, MSG_PEEK);
					if (c == '\n')
					{
						//运行到这里说明读到的是"\r\n"，这时候就要把这个字符从缓冲区中真正的读出来。然后算作一行读完
						recv(sock, &c, 1, 0);
					}

					else
					{
						//运行到这里说明窥探的字符并不是"\n",所以"\r"就是一行的结尾。
						c = '\n';
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
		while (strcmp(line_.c_str(), "\n"))//报头以'\n'为结尾，读到最后一行时，line_内的数据就是"\n"
		{
			line_ = "";//这里要注意每次进入循环时要将line_置为空，因为每行读的数据都还在line_里，要注意！
			RecvOneLine(line_);
			rq_head += line_;
		}

	}
	void RecvRequestText(string &text_, int len, string& param)
	{
		char c;
		int i = 0;
		while (i < len)
		{
			recv(sock, &c, 1, 0);
			text_.push_back(c);
		}
		param = text_;
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
    static void* RequestHandler(void *arg_)
    {
      //因为要实现读取请求并返回响应，故创建connect,response,request对象，分别调用他们的接口。
      int sock_ = *(int*)arg_;
      delete (int*)arg_;
      Connect *conn_ = new Connect(sock_);
      Request *rq_ = new Request();
      Response *rsp_ = new Response();

      //接下来的步骤就是读取http请求，先对请求中的首行进行解析
      conn_->RecvOneLine(rq_->rq_line);
      rq_->RequestLineParse();//旨在将rq_line的内容分别放在3个字段中

      int& code_ = rsp_->getcode();//获取状态吗

      if (!(rq_->CheckMethodLegal()) )//判断方法的合法性，顺便判断后面处理数据是否需要使用cgi
      {
        code_ = NOT_FOUND;
        goto end;
      }

      //运行到这里说明方法已经合法，现在进行uri的解析。
      rq_->UriParse();

      if (!rq_->CheckPathLegal( ))
      {
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
        code_=NOT_FOUND;
        goto end;
      }

      //报头报读完毕后，要检查是否有正文存在
      if (rq_->IsNeedReadText())
      {
        //读取正文
        conn_->RecvRequestText(rq_->rq_text,rq_->GetContentLength(),rq_->GetParam());
      }

      //到此读完请求
      Log(INFO,"handler request is done!!!");

   

end:
      if (code_!=OK)
      {
        //到这里说明请求出错，需要处理错误
        //HandlerError(sock_);
      }

      delete rsp_;
      delete rq_;
      delete conn_;

    }


};

#endif
