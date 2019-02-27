## 项目名称
搭建一个小型的http服务器
## 项目目标
服务器能接收用户发起的请求并做出响应，发送对应资源，或根据情况做异常处理。
## 应用技术
 1. 基于http协议进行通信
 2. 通过cgi模式处理用户请求中的参数
 3. 使用线程池控制服务器并发链接，提高链接性能
 4. 对于请求不存在的资源可以返回404错误码及页面
 5. 使用简单的shell脚本
 ## 效果图
 
  主页效果图
  ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/主页.png)
  
  访问某静态文件效果图
  ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/login.png)
  
  简单CGI效果图
  ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/简单cgi.png)
 
  访问错误，返回404效果图
 ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/404.png)
## 项目流程
### 服务器启动准备工作

1. 程序启动，构建一个HttpServer对象。

2. HttpServer对象负责服务器的启动（创建套接字/绑定地址信息/进入监听状态），同时创建含有5个线程的线程池。

3. 线程池创建出一个任务队列，用来接收http请求，5个线程创建好后依次进行互斥量的获取，判断队列是否有任务，无任务则陷入休眠。

4. 至此服务器准备就绪，开始让HttpServer接收HTTP请求。

### 服务器接收HTTP请求

5. 当有一个HTTP请求到来时，HTTPServer 使用accept接收到任务，将其封装成任务（new_socket和RequestHandler指针），插入到任务队列中。

6. 任务队列不为空，则唤醒一个线程对任务进行处理，调用Entry类中的RequestHandler方法。

7. 进入到Request方法后，先对HTTP请求进行解析。

8. 调用RecvOneLine函数，获取请求行(要对"\n","\n\r","\r"都转换为\n)  知识点MSG_PEEK(窥探)

9. 将其进行拆分后分别放入method，url，version变量中

10. 对method进行解析（method检查方式是比较是否是GET/POST）

11. url分别进行解析(先查'?',如果是GET方法且url中含'?'，则说明该请求是cgi请求。并将路径path和参数param分别保存)
   (url检查方式是调用stat函数，并对路径下文件是目录或可执行文件做分别处理,如果是目录则在后面补上/index.html,
    如果是可执行文件则将更换为cgi模式,顺便将后缀也识别出来,这里是为了标识响应报头中的Content-Type)
    同时记录资源大小,为了标识响应报头中的Content-Length.
    
11.5 检查path合法性,资源不存在将状态码记录为404,并跳转至end

12. 不正确则直接跳转到end，跳转前要记得先把报头读完

13. 到此请求行解读完毕,开始解读报头.调用RecvOneLine直到遇到空行即可获得所有报头.

14. 对报头的每一行进行读取,按照": "将各个键值对存入head_kv

15.读完报头,跳过空行,检查是否要读正文(只有POST方法才有正文,所以等同与判断method是否是"POST")

16.至此服务器解析HTTP请求结束,开始构建响应

### 服务器构建并返回HTTP响应

17.构建响应前,根据cgi的值有两种方式:非cgi响应和cgi响应

#### 服务器构建并返回非CGI响应

18.构建响应首行,默认版本为HTTP/1.0 ,状态码为200

19.构建响应报头,本项目中只构建Content-Length和Content-Type,这两个的值之前都已经url解析中记录过了,在resource_size和suffix变量中

20.打开path所指的资源,一会儿资源内容作为相应正文

21.发送时调用send依次向对端socket发送首行,报头,空行,正文,正文的长度用resource_size控制.

#### 服务器构建并返回CGI响应

18'. 构建两条匿名管道,再fork出子进程,两条管道对应父子进程间的数据传输.注意,由于我们要让子进程处理CGI,所以势必会调用exec函数族,一旦替换数据区和代码段,就无法找到操作描述符了(文件描述符表不会被替换,但是你无法知道那个文件描述符是你想要的),所以要先将管道描述符重定向到0,1.

19'. 父进程将参数param全部传入管道1,并等待子进程处理完毕,

20'. 子进程接收到参数后,因为exec函数会替换数据段和代码段,所以要先将参数的长度提前以环境变量的形式进行保存.然后调用exec函数执行path路径所指的文件.

21'. 子进程执行完毕后再通过管道2将数据结果反馈给父进程,父进程接收后用来组装成响应正文.

22. 到这就跟非CGI一样了,直接构建响应并返回.

CGI模式示意图

![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/CGI.png)
