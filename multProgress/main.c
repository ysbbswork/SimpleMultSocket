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


#define SRV_PORT 9999
#define BUFF_MAX 1024


void catch_child(int signum){
    while(waitpid(0,NULL,WNOHANG)>0);

    return;
}


void server_handler(int cfd){
    char buf[BUFF_MAX];
	while(1){
        int ret = read(cfd,buf,sizeof(buf));
        if (ret == 0){
            close(cfd);
            exit(1);
        }else if(ret<0){
            perror("read error.");
            close(cfd);
            exit(1);
        }
        printf("ret:%d,read str:%s",ret,buf);
        int i;
        for(i = 0; i<ret;i++){

            buf[i] = toupper(buf[i]);

        }
        write(cfd,buf,ret);
        write(STDOUT_FILENO,buf,ret);
	}
}


int main(){
    
    int ret;
	int lfd,clt_addr_len,cfd;
	struct sockaddr_in srv_addr,clt_addr;
    pid_t pid; 
    //memset(&srv_addr,0,sizeof(srv_addr));
    bzero(&srv_addr,sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SRV_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    lfd = socket(AF_INET,SOCK_STREAM,0);

    ret = bind(lfd,(struct sockaddr *)&srv_addr,sizeof(srv_addr));
    if(ret<0){
        perror("bind error.");
        exit(1);
    }

    listen(lfd,128);

    clt_addr_len = sizeof(clt_addr);


    while(1){
        
        cfd = accept(lfd,(struct sockaddr *)&clt_addr,&clt_addr_len);
        if(cfd<0){
           	if (errno == EINTR)
				continue;		//这里需要注意，在客户端终止后会返回-1，此时应该继续
			else
				perror("accept error");
				exit(1);
        }
        pid = fork();
        if(pid<0){
            perror("fork error.");
			exit(1);
        }else if(pid == 0){
            close(lfd);
			server_handler(cfd);
            
        }else{
			close(cfd);
			struct sigaction act;
            act.sa_handler = catch_child;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;

            ret = sigaction(SIGCHLD, &act, NULL);
            if(ret!=0){
                perror("sigaction error.");
                exit(1);
            }
			printf("%s","signal ing.");

        	
        }
    }
    
	

    return 0;
  
}
