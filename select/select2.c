/*
如果按select1的思路，

如果有三个需要监听的文件描述符，3,4,1023，

只有三个文件描述符，却要轮询将近1024次，

改进的方案是，额外在弄一个数组，将要监听的fd放入数组中，对数组进行轮询。

select2.c为改进后的select使用

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
    int listenfd,connfd,sockfd;
    char buf[BUFSIZ];  	//操作系统的宏，一般为8192，也可以自己定义
    char str[1024];
    struct sockaddr_in clie_addr,serv_addr;

	int client[FD_SETSIZE]; //自定义数组client,防止遍历1024个文件描述符 FD_SETSIZE默认为1024

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
        close(listenfd);
        exit(1);
    }
    ret = listen(listenfd,128);
    if (ret<0){
        perror("listen error.");
        close(listenfd);
        exit(1);
    }
	
    fd_set rset, allset;  //定义 读集合， 备份集合allset

    maxfd = listenfd;		//最大文件描述符

    //初始化client[]下标
    int maxi = -1;
    for(i = 0;i < FD_SETSIZE;i++){
        client[i] = -1;
    }

    FD_ZERO(&allset);				//清空监听集合
    FD_SET(listenfd,&allset);		//将待监听fd添加到监听集合中

    while(1){
        rset = allset;				//备份
        nready = select(maxfd+1,&rset,NULL,NULL,NULL);		//使用select 监听
        if (nready < 0){
            perror("select error.");
        }

        //处理listenfd 有事件发生
        if(FD_ISSET(listenfd,&rset)){				//说明新的客户端请求连接

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
			printf("received from %s at PORT %d\n",
				inet_ntop(AF_INET,&clie_addr.sin_addr,str,sizeof(str)),
				ntohs(clie_addr.sin_port));

           //找client[]中没有使用的位置
			for(i = 0;i < FD_SETSIZE; i++){   
				if(client[i] < 0){
					client[i] = connfd;
					break;
				}
			}

            
            if(maxi < i){
                maxi = i;           //保证maxi总是存着数组最后一个下标
            }
			
            FD_SET(connfd,&allset);		// 将新产生的fd 添加到监听集合中，监听数据读事件

            if(maxfd < connfd){
                maxfd = connfd;
            }

            
            if(0 == --nready){
                continue;               //说明select只返回一个listenfd,后续无需执行
            }
        }//结束处理listenfd事件发生


        //开始检测clientfd事件
        
        for (i = 0;i <= maxi; i++){	            //检测哪个client有数据就绪

           if((sockfd = client[i]) < 0){        
                continue;
           }
            if(FD_ISSET(sockfd,&rset)){         //找到满足读事件的那个fd
                if((n=read(sockfd,buf,sizeof(buf))) == 0){	//如果客户端已经关闭
                    close(sockfd);
                    FD_CLR(sockfd,&allset);				//将关闭的fd移出监听集合
                    client[i] = -1;                     //解除select对此文件描述符的监控
                }else if( n > 0 ){
                    for(j = 0; j < n;j++){
                        buf[j] = toupper(buf[j]);
                    }
                    write(sockfd, buf, n);
 //                   write(STDOUT_FILENO,buf,n);
                }
                if(--nready == 0){
                    break;                  //处理完有事件的fd了，跳出for,继续while
                }
            }
        }
    
    }
    close(listenfd);
    return 0;
}
