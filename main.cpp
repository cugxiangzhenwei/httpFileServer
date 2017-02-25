#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string>
#include<log.h>
using namespace std;
int startup(short *port);
void InitWorkHomeDir(const char * pszHomeDir);
void error_die(const char *sc);
void PrintSocketAddress(const struct sockaddr *address, FILE *stream);
void * accept_request(void *thread_arg);
//主函数入口,编译时
//gcc -W -Wall -lsocket -lpthread -o httpd httpd.c  在linux 可以去掉-lsocket
//参考开源tinyhttpd
int main(int argc, char *argv[]) {
	printf("httpFileServer start run...\n");
    int server_sock = -1;
    if (argv[1] == NULL)
        argv[1] = (char*)"80";
	
	int i = 1;
    char dirPath[128];
    getcwd(dirPath, 128);
	string strHomeDir = dirPath;
	while( i< argc -1)
	{
		if(strcmp(argv[i],"-d")==0)
		{
			strHomeDir = argv[i+1];	
			i+=2;
		}
		else
		{
			i++;
		}
	}
    short port = atoi(argv[1]);
    int client_sock = -1;
    struct sockaddr_in client_name;
    int client_name_len = sizeof (client_name);
    pthread_t newthread;
    //建立socket，监听端口
    server_sock = startup(&port);
    WriteFormatLog(LOG_TYPE_INFO, "httpd running on port %d\n", port);
    //获取程序目录  
    printf("current program Path: %s\n",strHomeDir.c_str());
    InitWorkHomeDir(strHomeDir.c_str());
	while (1) {
        client_sock = accept(server_sock,
                (struct sockaddr *) &client_name,
                (socklen_t*)&client_name_len);
        if (client_sock == -1)
            error_die("accept");
        PrintSocketAddress((struct sockaddr*) &client_name, stdout);
        /* accept_request(client_sock); */
          //printf("client_sock:%d\n",client_sock);
        //子线程处理http 请求
        if (pthread_create(&newthread, NULL, accept_request, (void*) client_sock) != 0)
            perror("pthread_create");
    }

    close(server_sock);
	printf("httpFileServer end run!\n");
    return (0);
}
