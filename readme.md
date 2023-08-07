## 问题：

1.一开始编译总是会出现函数或者静态成员变量未定义的情况，发现有一些静态成员未初始化
1.使用bind绑定之后发现子线程改变的http_conn里的writebuff回到主线程中不会生效
解决：采用lambda表达式
2.多个浏览器连接式发生错误，连接socket没有改变，发现write函数中返回len值了，但是最后len总是会返回-1，所以在主线程中最终总是会关闭连接
writev成功时总是返回0 失败返回-1，所以使用一个cnt累加读取的文件长度
3.编译会有warning，提示：
code/./pool/pthread_pool.h:35:8: warning: ‘ThreadPool::isclose’ will be initialized after
经过查找资料，发现了问题：变量声明与构造函数初始化时候的位置最好保持一致。
4.makefile有问题，编译会提示multiple definition，后来发现是写法不对
正确写法：只有这样会同名匹配.o和.cpp文件；
$(OBJS) : %.o: %.cpp
5.经过webbench测试发现速度很慢，只有1000pages/min  32600bytes/sec


## 待完成

* 定时器，能够实现客户端断开一段时间后，服务器端自动释放连接； done! 
* 实现服务器优雅退出，使用管道和信号机制在收到终止信号之后向服务器发送，服务器结束；
* 运用面向对象的思维优化代码，使得代码逻辑更清晰；
* 加入日志
* 加入MySQL允许用户注册登录