/*
poll实现多路IO复用的服务器例子

演示基本的poll使用

可以和select2进行对比，思路相似
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<errno.h>
#include<poll.h>
#define SERV_PROT 6668
#define MAXLINE 1024
#define OPEN_MAX 1024
#define POLL_ERR         (-1)
#define POLL_EXPIRE      (0)

int main(int argc, char *argv[])
{
    int i, j, n, nready;
    int ret;
    int listenfd,connfd,sockfd;

    
    char buf[MAXLINE]; 
    char str[INET_ADDRSTRLEN]; //INET_ADDRSTRLEN 为16
    
    struct sockaddr_in clie_addr,serv_addr;
    socklen_t clie_addr_len;
    
	struct pollfd client[OPEN_MAX]; //定义pollfd struct数组
    

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
	
    client[0].fd = listenfd;    //要监听的第一个文件描述符 存入client[0]
    client[0].events = POLLIN;  //listenfd 监听普通读事件

    for(i = 1;i < OPEN_MAX;i++){
        client[i].fd = -1;          //用-1 初始化client[]剩下的元素下标
    }

    int maxi = 0;           //client[]数组中有效元素中最大元素下标
    
    while(1){
        printf("while\n");
        nready = poll(client,(unsigned int)maxi+1,-1);		//使用poll 监听是否有链接请求 , -1为阻塞等待
        if(nready < 0){
            perror("poll");
            exit(EXIT_FAILURE);
        }
        
        //处理listenfd 有事件发生
        if(client[0].revents & POLLIN){				//说明新的客户端请求连接
            printf("listen fd\n");
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

           //找client[]中没有使用的位置
			for(i = 1; i < OPEN_MAX; i++){   
				if(client[i].fd < 0){
                    printf("client[%d].fd < 0\n",i);
					client[i].fd = connfd;      //找到client[]空闲的位置，放入accept返回的connfd
					break;
				}
			}

            //达到最大客户端数
            if(i == OPEN_MAX){
                perror("too many clients"); 
                exit(1);
            }
			
            client[i].events = POLLIN;      //设置刚返回的connfd，监听读事件
            if(i>maxi){
                maxi = i;               //更新client[]中最大元素下标
            }
            
            if(--nready <= 0){
                continue;               //没有更多的就绪事件，继续回到poll阻塞
            }
        }//结束处理listenfd事件发生

        printf("clientfd\n");
        //开始检测clientfd事件
     
        for (i = 0;i <= maxi; i++){	            //检测哪个client有数据就绪
           if((sockfd = client[i].fd) < 0){        
                continue;
           }
            if(client[i].revents & POLLIN){         //找到满足读事件的那个fd
                if((n=read(sockfd,buf,sizeof(buf))) == 0){	//如果客户端已经关闭
                    printf("client[%d] closed connection\n",i);
                    close(sockfd);
                    client[i].fd = -1;                     //解除poll对此文件描述符的监控
                }else if( n > 0 ){
                    for(j = 0; j < n;j++){
                        buf[j] = toupper(buf[j]);
                    }
                    write(sockfd, buf, n);
                    write(STDOUT_FILENO,buf,n);
                }else{
                    perror("read error.");
                    exit(1);
                }
                if(--nready == 0){
                    break;                  //处理完有事件的fd了，跳出for,继续while
                }
            }
        }
        
    printf("end  clientfd\n");
    }
    close(listenfd);
    return 0;
}
