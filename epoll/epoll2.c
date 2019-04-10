/*
epoll 基于非阻塞I/O事件驱动
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<errno.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<time.h>
#include<sys/socket.h>

#define LOG(fmt, args...)	do{\
			char _buf[1024] = {0};\
			snprintf(_buf, sizeof(_buf), "[%s:%s:%d][LOG_NORMAL]"fmt"\n",__FILE__,__FUNCTION__,__LINE__, ##args);\
			printf("%s",_buf);\
		}while(false)


#define SERV_PROT 6666
#define MAX_EVENTS 1024         //监听上限
#define BUFLEN 4096


void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);

//描述就绪文件描述符相关信息

struct myevent_s{
    int fd;                                             //要监听的文件描述符
    int events;                                         //对应的监听事件
    void *arg;                                          //泛型参数
    void (*call_back)(int fd, int events, void *arg);   //回调函数
    int status;                                         //是否在监听：1->在红黑树上（监听），0->不在监听
    char buf[BUFLEN];               
    int len;                
    long last_active;                                   //记录每次加入红黑树的 g_efd的时间  
};


//定义全局变量
int g_efd;                                  //保存epoll_create返回的文件描述符
struct myevenyt_s g_events[MAX_EVENTS+1];   //自定义结构体类型数组，+1 --> listen fd




//将结构体myevent_s 成员变量初始化
void eventset(struct myevent_s *ev, int fd, void(*call_back)(int,int,void *), void *arg)
{
    ev-> fd = fd;
    ev -> call_back = call_back;
    ev -> events = 0;
    ev -> arg = arg;
    ev -> status = 0;
    memset(ev->buf, 0, sizeof(ev->buf));
    ev -> len = 0;
    ev -> last_active = time(NULL);             //调用eventset函数的时间

    return；
}
    


void acceptconn(int lfd, int events, void *arg)
{
    struct sockaddr_in cin;         //客户端地址
    socklen_t len = sizeof(cin);
    int cfd, i;

    if((cfd = accept((lfd,(struct sockaddr *)&cin,&len)) == -1){
        if(errno != EAGAIN && errno != EINTR){
            //这两种情况不作为出错处理
        }
        LOG("accpet error:%s",strerror(errno));
        return;
    }

    do{
       for(i = 0; i<MAX_EVENTS;i++){                //从全局数组中找出一个空闲元素，跳出
            if(g_events[i].status == 0){
                break;
            }
        }

       if(i == MAX_EVENTS){
            LOG("max connetion limit[%d]",MAX_EVENTS);
            break;
       }

       int flag = 0;
       if((flag = fcntl(cfd,F_SETFL,O_NONBLOCK))< 0){ //将cfd也设置为非阻塞
            printf("fcntl nonblocking faild:%s",strerror(errno));
            break;
       }

       //给 cfd 设置一个myevent_s 结构体，回调函数 设置为 recvdata
       evenset(&g_events[i],cfd, recvdata,&g_events);
       evetadd(g_efd,EPOLLIN, &g_events[i]);            //将cfd添加到红黑树，监听读事件
    }while(0);

    printf("new connect[%s:%d][time:%ld],pos[%d]\n",
        inet_ntoa(cin.sin_addr),ntohs(cin.sin_port),g_events[i].last_active,i);
    return ;
}

//创建socket,初始化lfd
void initlistensocket(int efd, short port)
{
    struct sockaddr_in sin;

    int lfd = socket(AF_INET,SOCK_STREAM,0);
    fcntl(lfd, F_SETFL, O_NONBLOCK);         //将socket 设为非阻塞

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    int res = bind(lfd,(str))
    if(res < 0){
        perror("bind error");
        close(lfd);
        return;
    }
    
    res = listen(lfd,128);
    if (res<0){
        perror("listen error.");
        close(listenfd);
        return;
    }

    //将lfd放在数组最后一位
    eventset(&g_events[MAX_EVENTS],lfd,acceptconn,&g_events[MAX_EVENTS]);

    eventadd(efd,EPOLLIN,&g_events[MAX_EVENTS]);

    return;
}

int main(int argc, char *argv[])
{

    unsigned short port = SERV_PROT;
    if (argc == 2){
        prot = atoi(argv[1]);                   //如果用户指定了端口，用指定端口，否则默认
    }
    
    g_efd = epoll_create(MAX_EVENTS+1);         //创建红黑树，返回给全局g_efd
    if(g_efd <=0 ){
        LOG("create efd err:%s",strerror(errno));
    }

    initlistensocket(g_efd,port);               //初始化监听socket

    struct epoll_event events[MAX_EVENTS+1];    //保存已经满足就绪事件的文件描述符数组

    printf("server running:port[%d]\n",port);

    int checkpos = 0, i;
    while(1){

    
        //超时验证，每次测试100个连接，不测试listenfd 
        // 当客户端60秒内没有和服务器通信，则关闭此客户端连接
        long now = time(NULL);                                  //当前时间
        for(i = 0; i < 100; i++,checkpos++){
            if(checkpos == MAX_EVENTS){
                checkpos = 0;                                   //检查完毕，将监测点归零
            }
            if(g_events[checkpos].status != 1){                 //不在红黑树上
                continue;
            }
            long duraton = now - g_events[checkpos].last_active;//客户端不活跃的时间

            if (duration >= 60){
                close(g_events[checkpos].fd);                   //关闭与该客户端链接
                printf("[fd=%d] timeout\n", g_events[checkpos].fd);
                eventdel(g_efd,&g_events[checkpos]);            //将该客户端从红黑树 g_efd移除
            }
        }

        //监听红黑树 
        // g_efd，将满足事件的文件描述符加到events数组中，1秒没有事件满足，返回0
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS+1, 1000);
        if(nfd < 0){
            LOG("epoll_wait error,exit\n");
            break;
        }

        for(i = 0; i < nfd; i++){
            //使用自定义结构体myevent_s类型指针 ，接收 联合体data的void *ptr 成员
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;

            if((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)){         //读就绪事件
                ev->call_back(ev->fd, events[i].eve , ev->arg);
            }
            if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)){       //写就绪事件
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
        }
    }
    
    //退出前释放所有资源
    return 0;
}
