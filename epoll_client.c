#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>

int main(int argc,char* argv[])
{
	if(argc!=2)
	{
		printf("Usage :./select_client [port]");
		return 1;
	}

	//
	struct sockaddr_in client;
	client.sin_port = htons(atoi(argv[1]));
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	//创建套接字
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		perror("socket");
		return 1;
	}
	//连接服务器
	if(connect(sock,(struct sockaddr*)&client,sizeof(client))<0)
	{
		perror("connect");
		return 1;
	}
	for(;;)
	{
		//输入消息并刷新缓冲区
		printf("client >");
		fflush(stdout);
		//将消息读到buf里
		char buf[1024] = {0};
		read(0,buf,sizeof(buf)-1);
		//将消息写给文件描述符
		if(write(sock,buf,strlen(buf))<0){
			perror("write");
			continue;
		}
		//将服务器返回的消息写到buf里	
		int ret = read(sock,buf,sizeof(buf)-1);
		if(ret<0){
			perror("read");
			continue;
		}
		if(ret==0)
		{
			printf("server close\n");
			break;
		}
		printf("server:%s\n",buf);
	}
	close(sock);
	return 0;
}
