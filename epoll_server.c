#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>


int startup(int port)
{
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		perror("socket");
		exit(3);
	}
	int opt = 1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = htons(port);
	if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)
	{
		perror("bind");
		exit(4);
	}
	if(listen(sock,5)<0)
	{
		perror("listen");
		exit(5);
	}
	return sock;

}
void handler_events(int epfd,struct epoll_event revs[],int num,int listen_sock)
{
	int i = 0;
	struct epoll_event ev;
	for(;i<num;i++)
	{
		int fd = revs[i].data.fd;
		if(fd==listen_sock && (revs[i].events & EPOLLIN)){
			struct sockaddr_in client;
			socklen_t len = sizeof(client);
			int new_sock = accept(fd,(struct sockaddr*)&client,&len);
			if(new_sock<0)
			{
				perror("accept");
				continue;
			}
			//获得新连接不能直接读，否则会造成阻塞
			printf("get new link\n");

			ev.events = EPOLLIN;
			ev.data.fd = new_sock;
			epoll_ctl(epfd,EPOLL_CTL_ADD,new_sock,&ev);//将获得的新连接添加到epoll模型中

			continue;
		}
		if(revs[i].events & EPOLLIN){
			char buf[10240];
			ssize_t s = read(fd,buf,sizeof(buf)-1);
			if(s>0){
				buf[s]=0;
				printf("%s",buf);
				ev.events = EPOLLOUT;
				ev.data.fd = fd;
				epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
			}
			else if(s==0){
				printf("client quit");//客户端退出
				close(fd);//关闭文件描述符
				epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);//将文件描述符从epoll模型中去掉
			}
			else{
				perror("read");
				close(fd);//关闭文件描述符
				epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);//将文件描述符从epoll模型中去掉
			}
			continue;
		}
		if(revs[i].events & EPOLLOUT){
			const char* echo = "HTTP/1.1 200 OK\r\n\r\n<html>hello epoll !</html>\n";
			write(fd,echo,strlen(echo));
			//写完之后
			close(fd);//关闭文件描述符
			epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);//将文件描述符从epoll模型中去掉
		}
	}

}
//eopll_server 8080
int main(int argc,char* argv[])
{
	if(argc!=2)
	{
		printf("Usage:%s port\n",argv[0]);
		return 1;
	}

	int epfd = epoll_create(256);//绝对是3
	if(epfd<0)
	{
		perror("epoll_create");
		return 2;
	}
	int listen_sock = startup(atoi(argv[1]));

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = listen_sock;//把listen_sock托管起来

	epoll_ctl(epfd,EPOLL_CTL_ADD,listen_sock,&ev);

	struct epoll_event revs[128];
	int n = sizeof(revs)/sizeof(revs[0]);
	int timeout = 1000;
	int num = 0;

	for(;;){
		switch((num = epoll_wait(epfd,revs,n,timeout))){
			case -1:
				perror("epoll_wait");
				break;
			case 0:
				printf("timeout\n");
				break;
			default:
				handler_events(epfd,revs,num,listen_sock);
				break;

		}
	}

	close(epfd);
	close(listen_sock);
	return 0;
}
