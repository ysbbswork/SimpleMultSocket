/*
epoll实现多路IO复用的服务器例子

演示基本的epoll使用

LT 水平触发模型例子。

*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<errno.h>
#include<sys/epoll.h>



#define SERV_PROT 6666
#define MAXLINE 1024

#define OPEN_MAX 5000


int main(int argc, char *argv[])
{
    int i, nready, efd, res;
    int n, num = 0;
    int listenfd,connfd,sockfd;
    
    char buf[MAXLINE]; 
    char str[INET_ADDRSTRLEN]; //INET_ADDRSTRLEN 为16
    
    struct sockaddr_in clie_addr,serv_addr;
    socklen_t clie_addr_len;
    

    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd < 0){
        perror("listen error.");
        exit(1);
    } 
    int opt = 1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PROT);
    res = bind(listenfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(res < 0){
        perror("bind error");
        close(listenfd);
        exit(1);
    }
    res = listen(listenfd,128);
    if (res<0){
        perror("listen error.");
        close(listenfd);
        exit(1);
    }
	//创建epoll红黑树
    efd = epoll_create(OPEN_MAX);
    if(efd == -1){
        perror("epoll_create error.");
        exit(1);
    }

    //往红黑树上挂监听fd
    struct epoll_event tep, ep[OPEN_MAX];   //tep:epoll_ctl参数 ep[]:epoll_wait参数

    tep.events = EPOLLIN;
    tep.data.fd = listenfd; //  指定lfd监听事件为读
    
    //将lfd及对应的结构体设置到树上，efd可找到该树
    res = epoll_ctl(efd,EPOLL_CTL_ADD,listenfd,&tep);
    if(res == -1){
        perror("epoll_ctl error.");
        exit(1);
    }

    for(;;){
        // epoll 为server阻塞监听事件，ep为struct epoll_event类型数组，OPEN_MAX为数组容量, 
        // -1表永久阻塞
        nready = epoll_wait(efd,ep,OPEN_MAX,-1);
        if(nready == -1){
            perror("epoll error");
            exit(EXIT_FAILURE);
        }

        for(i = 0;i < nready; i++){
            if(!ep[i].events & EPOLLIN){
                continue;
            }
            //处理listenfd 有事件发生
            if(ep[i].data.fd == listenfd){
                clie_addr_len = sizeof(clie_addr);
    			 // accept 建立连接，因为在调的时候，就说明有请求来了，所以不会阻塞
                connfd = accept(listenfd,(struct sockaddr *)&clie_addr,&clie_addr_len);
                if(connfd<0){

               	    if (errno == EINTR){
    				    continue;		//这里需要注意，在客户端终止后会返回-1，此时应该继续
    				    perror("accept continue");
                    }
                    else{
    				    perror("accept error");
    				    exit(1);
                    }
                }
    			printf("received from %s at PORT %d\n",
    				inet_ntop(AF_INET,&clie_addr.sin_addr,str,sizeof(str)),
    				ntohs(clie_addr.sin_port));

               tep.events = EPOLLIN; tep.data.fd = connfd;
               res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);   //加入红黑树
               if(res == -1){
                perror("epoll_ctl error.");
                exit(1);
               }
            }
            else{//不是lfd
                sockfd = ep[i].data.fd;
                n = read(sockfd,buf,sizeof(buf));
        
                if(n == 0){	//如果客户端已经关闭
                    //从红黑树摘除
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                    if (res == -1){
                        perror("epoll_ctl error.");
                        exit(1);
                    }
                    printf("client[%d] closed connection\n",i);
                    close(sockfd);
                }else if( n > 0 ){
                    for(i = 0; i < n;i++){
                        buf[i] = toupper(buf[i]);
                    }
                    write(sockfd, buf, n);
                    write(STDOUT_FILENO,buf,n);
                }else{
                    perror("read error.");
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                    close(sockfd);
                    exit(1);
                }

           }
        }
    }
    close(listenfd);
    return 0;
}
