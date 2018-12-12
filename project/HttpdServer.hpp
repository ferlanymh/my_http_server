#ifndef __HTTPD_SERVER_HPP__
#define __HTTPD_SERVER_HPP__

//服务器的初始化及启动
#include"ProtocolUtil.hpp"

class HttpdServer
{
    private:
        int listen_sock;//监听scoket套接字，用于接收客户端发来的请求
        int port;//端口号
    public:
        HttpdServer(int port_)
            :listen_sock(-1)
             ,port(port_)
    {}



        void InitServer()//初始化服务器
        {
            //1.创建套接字
            //2.为服务器绑定地址信息
            //3.启动监听
            //4.获取客户端请求
            //5.收发数据
            //

            listen_sock = socket(AF_INET,SOCK_STREAM,0);//1.创建套接字
            if (listen_sock < 0)//创建套接字失败，写入日志并返回错误码2
            {
                Log(ERROR,"create socket failed!!!");
                exit(2);
            }

            int opt_ = 1;
            setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&opt_,sizeof(opt_));


            struct sockaddr_in local_;//创建存储服务端地址信息的结构体
            local_.sin_family = AF_INET;//套接字类型
            local_.sin_port =htons(port);//端口号
            local_.sin_addr.s_addr = INADDR_ANY;
            //解释一下为什么使用INADDR_ANY:一个服务器可能有多个网卡，故有多个IP。
            //当每个IP接收到数据时都应该交由服务器处理，而不是绑定某个固定的IP地址，导致服务器上别的IP无法受理请求。
            //让系统自己去检测并绑定地址也能有效规避错误，故推荐这样使用。
            


            if (bind(listen_sock,(struct sockaddr*)&local_,sizeof(local_)) < 0)//2.绑定地址信息,失败写入日志，返回错误码3
            {
                Log(ERROR,"create bind failed!!!");
                exit(3);
            }

            if (listen(listen_sock,5) < 0)//3.启动监听，失败写入日志，返回错误码4
            {
                Log(ERROR,"create listen failed!!!");
                exit(4);
            }

            Log(INFO,"InitServer Success!!");

        }
        
        
        void Start()//启动服务器，完成接收客户端请求并创建线程处理请求的功能
        {
            Log(INFO,"start to recv data!!!");
            while (1)
            {
                struct sockaddr_in peer_;//创建一个存储请求信息的结构体。
                socklen_t len_=sizeof(peer_);
                int sock_ = accept(listen_sock,(struct sockaddr*)&peer_,&len_);
                //4.开始接收客户端请求,并创建一个新的套接字sock_来处理请求。
                if (sock_ < 0)
                {
                    Log(WARNING,"accept failed!  keep listening!!");
                    continue;
                }
                
                
                //运行到这里说明成功接收到客户端请求，开始创建新线程来处理
                pthread_t tid_;
                
                int *tmp=new int;
                
                *tmp = sock_;
                
                Log(INFO,"Get a new client! Create a new thread handle the request!!");
                int ret=pthread_create(&tid_, NULL , Entry::RequestHandler , (void*)tmp );
                //具体实现在protocolutil头文件中
                if (ret!=0)
                {
                    Log(WARNING ,"create thread failed!!!");
                    continue;
                }
            }   
        }


        ~HttpdServer()
        {
            if (listen_sock != -1)
            {
                close(listen_sock);
            }
        }
};

#endif
