#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<string.h>
#include<strings.h>
#include<ctype.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/wait.h>
#include<errno.h>
#include<pthread.h>

#define SRV_PORT 8888
#define BUFF_MAX 1024


struct s_info{
	struct sockaddr_in cliaddr;
	int connfd;
};


void *server_handler(void *arg)
{
	struct s_info *ts = (struct s_info*)arg;
	char buf[BUFF_MAX];
	char str[INET_ADDRSTRLEN];
	
	while(1){
        int ret = read(ts->connfd,buf,sizeof(buf));
		//客户端关闭
        if (ret == 0){
			printf("the client %d closed.\n",ts->connfd);
            close(ts->connfd);
		    break;	
        }
		//其它错误
		else if(ret<0){
            perror("read error.");
            close(ts->connfd);
            exit(1);
        }
		
        printf("received from %s at PORT %d.\n",
        		inet_ntop(AF_INET,&((ts->cliaddr).sin_addr),str,sizeof(str)),
        		ntohs(ts->cliaddr.sin_port));
        int i;
        for(i = 0; i<ret;i++){

            buf[i] = toupper(buf[i]);

        }
        write(ts->connfd,buf,ret);
        write(STDOUT_FILENO,buf,ret);
	}
	
	return (void*)0;
}






int main(){
    
    int ret;
	int lfd,clt_addr_len,cfd;
	struct sockaddr_in srv_addr,clt_addr;

    //memset(&srv_addr,0,sizeof(srv_addr));
    bzero(&srv_addr,sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SRV_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    lfd = socket(AF_INET,SOCK_STREAM,0);

    bind(lfd,(struct sockaddr *)&srv_addr,sizeof(srv_addr));

    listen(lfd,128);

    clt_addr_len = sizeof(clt_addr);

	struct s_info ts[256]; //结构体数组，限定，最多256个线程

	pthread_t tid;

	int i =0;
	
    while(1){
        
        cfd = accept(lfd,(struct sockaddr *)&clt_addr,&clt_addr_len);
        if(cfd<0){
           	if (errno == EINTR)
				continue;		//这里需要注意，在客户端终止后会返回-1，此时应该继续
			else
				perror("accept error");
				exit(1);
        }
		ts[i].cliaddr = clt_addr;
		ts[i].connfd = cfd;

		pthread_create(&tid,NULL,server_handler,(void*)&ts[i]);

		pthread_detach(tid);
		i++;
    }
    
	

    return 0;
  
}
