#include <stdio.h>//printf
#include <string.h>//字符串处理
#include <unistd.h>//write系统调用
#include <stdlib.h>//exit函数
#include <string>
struct HTTP_RES_HEADER//保持相应头信息
{
    int status_code;//HTTP/1.1 '200' OK
    char content_type[128];//Content-Type: application/gzip
   	long long content_length;//Content-Length: 11683079
	char szLocation[1024]; // http code 302 will has this data
};

void GetHeader(std::string & strHeaders,int client_socket);
struct HTTP_RES_HEADER parse_header(const char *response);
void get_ip_addr(char *host_name, char *ip_addr);
