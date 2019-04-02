/*
基于select的IO复用服务器
体现基本的select用法
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<errno.h>

#define SERV_PROT 6666

int main(int argc, char *argv[])
{
    int i, j, n, nready;
    int ret;
    int maxfd = 0;
    int listenfd,connfd;
    char buf[BUFSIZ];  	//操作系统的宏，一般为8192，也可以自己定义
    struct sockaddr_in clie_addr,serv_addr;

    socklen_t clie_addr_len;

    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd<0){
        perror("listen error.");
        exit(1);
    } 
    int opt = 1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PROT);
    ret = bind(listenfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(ret < 0){
        perror("bind error");
        exit(1);
    }
    ret = listen(listenfd,128);
    if (ret<0){
        perror("listen error.");
        exit(1);
    }
	
    fd_set rset, allset;  //定义 读集合， 备份集合allset

    maxfd = listenfd;		//最大文件描述符

    FD_ZERO(&allset);				//清空监听集合
    FD_SET(listenfd,&allset);		//将待监听fd添加到监听集合中

    while(1){
        rset = allset;				//备份
        nready = select(maxfd+1,&rset,NULL,NULL,NULL);		//使用select 监听
        if (nready < 0){
            perror("select error.");
        }
        if(FD_ISSET(listenfd,&rset)){				//listenfd满足监听的 读事件

            clie_addr_len = sizeof(clie_addr);
			 // accept 建立连接，因为在调的时候，就说明有请求来了，所以不会阻塞
            connfd = accept(listenfd,(struct sockaddr *)&clie_addr,&clie_addr_len); 
            if(connfd<0){
           	    if (errno == EINTR){
				    continue;		//这里需要注意，在客户端终止后会返回-1，此时应该继续
                }
                else{
				    perror("accept error");
				    exit(1);
                }
            }
            FD_SET(connfd,&allset);		// 将新产生的fd 添加到监听集合中，监听数据读事件

            if(maxfd < connfd){
                maxfd = connfd;
            }
            if(0 == --nready){
                continue;               //说明select只返回一个listenfd,后续无需执行
            }
        }
        for (i = listenfd+1;i <= maxfd; i++){	//处理满足读事件的fd
            if(FD_ISSET(i,&rset)){				//遍历找到满足读事件的那个fd
                if((n=read(i,buf,sizeof(buf))) == 0){	//如果客户端已经关闭
                    close(i);
                    FD_CLR(i,&allset);				//将关闭的fd移出监听集合
                }else if( n > 0 ){

                    for(j = 0; j < n;j++){
                        buf[j] = toupper(buf[j]);
                    }
                    write(i, buf, n);
                }
            }
        }
    
    }
}
