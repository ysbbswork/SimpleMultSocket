本项目包含几个例子，用来帮助学习并发服务器。

- 多进程并发服务器
- 多线程并发服务器
- IO复用
  - select
  - poll
  - epoll
- 注意事项

# 多进程并发服务器

基本框架：

```
1.Socket();
2.Bind();
3.Listen();
4.While(1){
	if (pid == 0){
       	close(lfd);
    	read();
    	//do something
    	write();
	}else if(pid > 0){
    	close(cfd);
    	contine;
	}
}
```

但是要考虑父进程回收子进程资源，避免僵尸进程。

父进程：因为使用waitpid会阻塞在那里循环等待子进程被回收，新的请求来了没办法处理。所以采取信号量捕捉。

```
	close(cfd);
	注册信号捕捉函数： SIGCHID
	在回调函数中，完成子进程回收
		while（waitpid());

```

# 多线程并发服务器

基本框架：

```
1.Socket();
2.Bind();
3.Listen();
4.While(1){
       	cfd = Accpet(lfd,);
       	pthread_create(&tid,NULL,tfn,NULL);
       	pthread_detach(tid);
}
5.子线程：
void *tfn(void *arg)
{
    close(lfd);
    read(cfd);
    //do something
    write(cfd);
}
```

# IO复用

演变：

- 阻塞
- 非阻塞忙轮询
- 相应式——多路IO转接
  - select
  - poll
  - epoll

## select

基本框架

```
1.lfd = socket();  //创建套接字
2.bind();			//绑定地址结构
3.listen();			//设置监听上限
4.fd_set rset，allset;		//创建r监听集合
5.FD_ZERO(&allset);	//将r监听集合清空
6.FD_SET(lfd,&allset); //将lfd添加至读集合中

while(1){
    rset = allset;			//保存监听集合
	ret = select(lfd+1,&rset,NULL,NULL,NULL); //监听文件描述符集合对应事件
    if (ret>0){								//有监听的描述符满足对应事件
        if(FD_ISSET(LFD,&RSET)){		//1 在，0 不在
            cfd = accept();				//建立连接，返回用于通信的文件描述符
            FD_SET(cfd,&allset);		//添加到监听通信描述符集合中
        }
        for (i = ldf+1; i<=最大文件描述符;i++){
            FD_ISSET(i,&rset);		//有read、write事件
            read();
            小写——大写;
            write();
        }
    }
}

```

### 优缺点

缺点：

- 无法直接定位监听文件描述符事件，需要挨个循环查询

- 如果按我们这个思路，如果有三个需要监听的文件描述符，3,4,1023，只有三个文件描述符，却要轮询将近1024次，如select1.c

  改进的方案是，额外在弄一个数组，将要监听的fd放入数组中，对数组进行轮询。

  - 即，检测满足条件的fd，自己要添加业务逻辑，提高编码难度
  - select 监听上限，受文件描述符限制，最大1024个，如果想改，要重新修改编译内核。

  改进的方案见select2.c

优点：

- 跨平台：win、linux、macOS、Unix...

## poll

poll 是对select的改进

```
int poll(struct pollfd *fds, nfds_t nfds, int timeout); 

	fds: 监听的文件描述符【数组】
	nfds: 监听数组的实际有效个数
	timeout:
			>0:超时时长，单位毫秒
			-1：阻塞等待
			0：不阻塞
			
	返回值：返回满足对应监听事件的文件描述符个数
```

### 优缺点

优点：

- 自带数组结构
- 可以 将监听事件集合 和 返回事件集合 分离
- 可以拓展监听上限，超出1024限制

缺点：

- 不能跨平台，只能在类Unix，Linux使用
- 无法直接定位满足监听文件描述符

## epoll

```
int epoll_create(int size);
	size:创建的红黑树的监听节点数量。（仅供内核参考）
	返回值:指向新创建的红黑树的根节点的fd
	失败：返回-1
```

- size:创建的红黑树的监听节点数量。（仅供内核参考）
- 返回值:指向新创建的红黑树的根节点的fd。失败：返回-1

```
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```

- epfd:epoll_create函数的返回值。epfd
- op：对该监听红黑树的操作。
  EPOLL_CTL_ADD 添加fd到 监听红黑树
  	EPOLL_CTL_MOD 修改fd在监听红黑树上的监听事件
  	EPOLL_CTL_DEL 将一个fd 从监听红黑树上摘下（取消监听）
- fd：待监听的fd
- event：本质struct epoll_event 结构体 地址
  成员 events:
    		EPOLLIN/EPOLLOUT/EPOLLERR/EPOLLET
    	成员 data:联合体：
    		int fd;  对应监听事件的fd
    		void *ptr;
    		uint32_t u32;
    		uint64_t u64;
- 返回值：成功0；失败：-1 errno

```
int epoll_wait(int epfd,struct epoll_event *events, int maxevents, int timeout);
```

- epfd:epoll_create函数的返回值。epfd
- events：传出参数，【数组】，满足监听条件的哪些 fd 结构体。
- maxevents：数组 元素的总个数。1024
  - struct epoll_event events[1024];
- timeout:
  - -1 阻塞
  - 0 不阻塞
  - \>0 超时时间，毫秒
- 返回值
  - \>：满足监听的总个数 可以用作循环上限
  - 0：没有fd满足监听事件
  - -1：失败，errno

### 优缺点

- 优点
  - 高效，突破1024文件描述符限制
- 缺点
  - 不能跨平台。linux

### 特点

epoll是Linux下多路复用IO接口select/poll的增强版本，它能显著提高程序在大量并发连接中只有少量活跃的情况下的系统CPU利用率，因为它会复用文件描述符集合来传递结果而不用迫使开发者每次等待事件之前都必须重新准备要被侦听的文件描述符集合，另一点原因就是获取事件的时候，它无须遍历整个被侦听的描述符集，只要遍历那些被内核IO事件异步唤醒而加入Ready队列的描述符集合就行了。

## epoll 事件模式

EPOLL事件有两种模型：

Edge Triggered (ET) 边缘触发只有数据到来才触发，不管缓存区中是否还有数据。

Level Triggered (LT) 水平触发只要有数据都会触发。

- 水平触发LT：
  - 缓存区剩余未读尽的数据会导致 epoll_wait 返回
- 边缘触发ET：
  - 缓存区剩余未读尽的数据不会导致 epoll_wait 返回，新的事件满足，才会触发。
  - event.events = EPOLLIN|EPOLLET ;

LT模式即Level Triggered工作模式。

与ET模式不同的是，以LT方式调用epoll接口的时候，它就相当于一个速度比较快的poll，无论后面的数据是否被使用。

**LT(level triggered)：LT是缺省的工作方式，并且同时支持block和no-block socket。**在这种做法中，内核告诉你一个文件描述符是否就绪了，然后你可以对这个就绪的fd进行IO操作。如果你不作任何操作，内核还是会继续通知你的，所以，这种模式编程出错误可能性要小一点。**传统的select/poll都是这种模型的代表。**

**ET(edge-triggered)：ET是高速工作方式，只支持no-block socket。**在这种模式下，当描述符从未就绪变为就绪时，内核通过epoll告诉你。然后它会假设你知道文件描述符已经就绪，并且不会再为那个文件描述符发送更多的就绪通知。请注意，如果一直不对这个fd作IO操作(从而导致它再次变成未就绪)，内核不会发送更多的通知(only once).

大部分epoll模型为非阻塞的边缘触发模型。

### epoll边缘触发模型ET

epoll的ET模式：高效模式，但是只支持 非阻塞模式，

将 fd 设置成非阻塞，epoll不会阻塞，read 时也不会阻塞。

```
struct epoll_event event;
event.events = EPOLLIN|EPOLLET;
epoll_ctl(epfd,EPOLL_CTL_ADD,cfd &event);
int flg = fcntl(cfd, F_GETFL);
flg |= O_NONBLOCK;
fcntl(cfd,F_SETFL,flg);
```



## epoll反应堆模型

架构：	epoll ET模式 + 非阻塞 + void *ptr

原epoll模型：

- socket、bind、listen

- epoll_create 创建监听红黑树 

- 返回 epfd - epoll_ctl() 向树上添加一个监听fd 

- epoll_wait 监听 - 对应监听fd有事件产生

- 返回 监听 满足数组。- 判断返回数组元素 

- lfd 满足 

- Accept 

- cfd 满足 

- read() 

- 小写转大写（业务） 

- wirte 回去。

  

反应堆模型：

- socket、bind、listen
- epoll_create 创建监听红黑树 
- 返回 epfd - epoll_ctl() 向树上添加一个监听fd 
- epoll_wait 监听 - 对应监听fd有事件产生
- 返回 监听 满足数组。- 判断返回数组元素 
- lfd 满足 
- Accept 
- cfd 满足 
- read() 
- 小写转大写（业务） 
- **cfd 从监听红黑树书上摘下**
- **epoll_ctl() 修改监听 cfd 写事件 -- EPOLLOUT**
- **回调函数**
- **重新放回红黑树监听写事件**
- **等待 epoll_wait 返回  -- 说明cfd 可写**
- wirte 回去
- **cfd从红黑树上摘下**
- **修改监听事件 --EPOLLIN**
- **重新放回红黑树监听读事件**



因为对面可能进行会有半关闭状态，或者对面处理太慢滑动窗口写满，所以wirte也会被阻塞，这样的改进，保证只有可以写的时候，才写，增加效率。

反应堆：不但要监听 cfd 的读事件、还要监听cfd的写事件。



# 其它

## 设置端口复用setsockopt

主动关闭掉server之后，即svr端主动断开，需要经历TIME_WAIT，等待2MSL时长，但服务器重启时候，等待这段时间对服务影响很大，用户体验不好，设置端口复用的作用就体现在这里。

此时重启server，会发生bind错误：`bind error.: Address already in use `

​	在server的TCP连接没有完全断开之前不允许重新监听是不合理的。因为TCP连接没有完全断开指的是connfd（127.0.0.1:6666）没有完全断开，而我们重新监听的是listenfd（0.0.0.0:6666）。虽然是占用同一个端口，但IP地址时不同的，connfd 对应的是某个客户端通信的一个具体的IP地址，而listenfd对应的是wildcard address,解决这个问题的方法是使用setsockopt()设置socket描述符的选项SO_REUSEADDR为1，表示允许创建端口号相同但IP地址不同的多个socket描述符。

在server代码的socket()和bind()调用之间插入如下代码：

```
int opt = 1;
setsockopt(listenfd,SOL_SOCKET,SO_REUSERADDR,&opt,sizeof(opt));

```



### 检查read返回值

read的返回值有几种情况，在使用中 需要注意，编码过程中反复用到。

- read的返回值:
  - `> 0`实际读到的字节数
  - `= 0`已经读到结尾（对端已经关闭）【重点】
  - `-1`应进一步 判断errno的值：
    - errno = EAGAIN or EWOULDBLOCK：设置了非阻塞方式 读。没有数据到达。
    - errno = EINTR 慢速系统调用被中断
    - error = ECONNRESET 连接被重置
    - errno = "其它" 异常

问：为什么read等于零的时候，表示客户端关闭了？

```
ret = read (cfd,buf,sizeof(buf));
if(ret == 0){
    close(fd);
    exit(1);
}

```

测试的时候，客户端发送空，也是有字节发来的。。真的只有关闭时才会是ret == 0，为什么呢？

答：

以用 read 函数读取 socket 为例：对于 socket 连接的*阻塞*读请求， 

1: 如果对端没有写数据，那么会一直阻塞直到后面两种情况发生，

2: 如果接收到对端写的数据，那么会返回这次请求接收的数据的字节数，

3: 如果对端关闭连接\结束写操作(即发送 FIN)，那么会返回 0 。你说的客户端发送空字符的情况，说法是不准确的， socket 上发送或接受的都是字节流，要么有数据，要么没数据，无所谓数据的内容，只看数据的字节数

### accept

系统的**慢系统调用会被信号打断**，而不是所有的系统调用都会被信号打断。

慢系统调用来描述那些可能永远堵塞的系统调用，如：accept，read，等。

当慢系统调用接收到信号的时候，系统调用可能返回一个EINTR错误，多进程的服务器中，为了避免accept被子进程信号打断，需要自己在accept返回-1的情况下，判断errorn是否为EINTR，并且重启accept。

在unp中，已经将accept的这项措施在错误处理封装为Accept了，直接调用unp.h，所以很多人在学习的时候没有注意到。

第一种方法： 用continue进入for的下一次循环，从而重启被中断的系统调用；

```
for( ; ; ) 
{
    clilen = sizeof(cliaddr);
    if((connfd = accept(listenfd, (SA *)&cliaddr, &clilen)) < 0) {
        if(errno == EINTR) 
            continue;
        else 
            err_sys("accept error");
    }
}

```

或者 用goto来实现一样的功能，也同样让被中断的系统调用重启；

```
Again:
for( ; ; ) 
{
    clilen = sizeof(cliaddr);
    if((connfd = accept(listenfd, (SA *)&cliaddr, &clilen)) < 0) {
        if(errno == EINTR) 
            goto Again;
        else 
            err_sys("accept error");
    }
}

```

## 查看机器可以打开的文件描述符上限

- 可以使用cat命令查看一个进程可以打开的socket描述符上限。
  - `cat /proc/sys/fs/file-max` 
  - 当前计算机所能打开的最大文件个数，受硬件影响
- `ulimit -a`
  - 当前用户下，进程最大打开的文件描述符个数，默认为1024

如有需要，可以通过修改配置文件的方式修改该上限值。

   ` sudo vi /etc/security/limits.conf`

​    在文件尾部写入以下配置,soft软限制，hard硬限制。如下所示。

```
* soft nofile 65536			->修改默认值

* hard nofile 100000		->命令修改上限值

```

需要注销用户（或重启），使其生效。