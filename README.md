## 项目名称
搭建一个小型的http服务器，服务器能提供在线画图，整型间简单运算的服务
## 项目目标
服务器能接收用户发起的请求并做出响应，发送对应资源，或根据情况做异常处理。
## 应用技术
 1. 服务器能处理GET/POST请求，并根据请求资源的路径返回响应资源
 2. 能处理静态请求，能根据 URL 返回一个服务器上的静态文件（如画图板界面）
 3. 基于CGI技术处理HTTP请求中的参数，使用创建子进程和匿名管道处理CGI程序
 4. 使用线程池管理多个链接请求，提高了服务器的并发性
 5. 对访问不存在的资源，能返回404错误码和一个错误页面
 
 ## 效果图
 
  主页效果图
  ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/主页.png)
  
  访问某静态文件效果图
  ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/login.png)
  
  在线画图板效果图
  ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/%E7%94%BB%E5%9B%BE%E6%9D%BF.png)
  
  简单CGI效果图
  ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/简单cgi.png)
 
  访问错误，返回404效果图
 ![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/404.png)
## 项目流程
### 服务器启动准备工作

1. 程序启动，构建一个HttpServer对象。

2. HttpServer对象负责服务器的启动（创建套接字/绑定地址信息/进入监听状态），同时创建含有5个线程的线程池。

3. 线程池内含一个任务队列，用来接收http请求，先初始化条件变量和互斥量，5个线程创建好后先线程分离，再依次进行互斥量的获取，判断队列是否有任务，无任务则挂起等待。

4. 至此服务器准备就绪，开始让HttpServer接收HTTP请求。

### 服务器接收HTTP请求

5. 当有一个HTTP请求到来时，HTTPServer 使用accept接收到任务，将其封装成任务（new_socket和RequestHandler指针），插入到任务队列中。

6. 同时唤醒一个线程对任务进行处理，调用Entry类中的RequestHandler方法。

7. 进入到Request方法后，先对HTTP请求进行解析。

8. 调用RecvOneLine函数，获取请求行(要对"\n","\r\n","\r"都转换为\n)  知识点MSG_PEEK(窥探)

9. 使用stringstream按照空格将请求行拆分后分别放入method，url，version变量中

10. 对method进行解析（method检查方式是比较是否是GET/POST）

11. url分别进行解析(先查'?',如果是GET方法且URL中含'?'，则说明该请求是cgi请求。并将路径path和参数param分别保存,如果是POST则URL全部是path)
   (如果path最末是‘/’则是目录，在后面补上/index.html,path检查方式是调用stat函数，并对路径下文件是目录或可执行文件做分别处理,如果
    如果是可执行文件则将更换为cgi模式,顺便将后缀也识别出来,这里是为了标识响应报头中的Content-Type)
    同时记录资源大小,为了标识响应报头中的Content-Length.
    
11.5 检查path合法性,资源不存在将状态码code记录为404,并跳转至end

12. 不正确则直接跳转到end，跳转前要记得先把报头读完

13. 到此请求行解读完毕,开始解读报头.调用RecvOneLine直到遇到空行即可获得所有报头.

14. 对报头的每一行进行读取,按照": "将各个键值对存入head_kv

15.读完报头,跳过空行,检查是否要读正文(只有POST方法才有正文,所以等同与判断method是否是"POST")

16.至此服务器解析HTTP请求结束,开始构建响应

### 服务器构建并返回HTTP响应

17.构建响应前,根据cgi的值有两种方式:非cgi响应和cgi响应

#### 服务器构建并返回非CGI响应

18.构建响应首行,默认版本为HTTP/1.0 ,状态码是之前的code,状态码描述是code所对应的描述。

19.构建响应报头,本项目中只构建Content-Length和Content-Type,这两个的值之前都已经url解析中记录过了,在resource_size和suffix变量中

20.打开path所指的资源,一会儿资源内容作为相应正文

21.发送时调用send依次向对端socket发送首行,报头,空行,正文,正文的长度用resource_size控制.

#### 服务器构建并返回CGI响应

18'. 构建两条匿名管道,再fork出子进程,两条管道对应父子进程间的数据传输.注意,由于我们要让子进程处理CGI,所以势必会调用exec函数族,一旦替换数据区和代码段,就无法找到操作描述符了(文件描述符表不会被替换,但是你无法知道那个文件描述符是你想要的),所以要先将管道描述符重定向到0,1,还要将参数的长度提前以自定义环境变量的形式进行保存.

19'. 父进程将参数param全部传入管道1,并等待子进程处理完毕,

20'. 子进程接收到参数后,然后调用exec函数执行path路径所指的文件.

21'. 子进程执行完毕后再通过管道2（cout标记的管道2）将数据结果反馈给父进程,父进程接收后用来组装成响应正文.

22. 到这就跟非CGI一样了,直接构建响应并返回.

CGI模式示意图

![Image_text](https://github.com/ferlanymh/my_http_server/blob/master/CGI.png)
