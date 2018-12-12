#include"HttpdServer.hpp"

static void return_error(string s)
{
    std::cout<< "usage" << s << "port"<<std::endl;
}


int main(int argc,char *argv[])
{
    if (argc != 2)//进入此处说明命令行参数有误，返回错误码1
    {
        return_error(argv[0]);
        exit(1);
    }

    HttpdServer *serp = new HttpdServer(atoi(argv[1]));
    //构建一个HttpdServer对象，该对象的定义在HttpdServer.hpp中,并将端口号传给该对象
    
    serp->InitServer();
    serp->Start();

    delete serp;//
    return 0;
}

