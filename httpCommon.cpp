#include"httpCommon.h"
#include <arpa/inet.h>//ip地址处理
#include <netdb.h>//查询DNS
void GetHeader(std::string & strHeaders,int client_socket)
{
	int mem_size = 4096;
    int length = 0;
    int len;
    char *buf = (char *) malloc(mem_size * sizeof(char));
    char *response = (char *) malloc(mem_size * sizeof(char));
    buf[0] ='\0';
	buf[0] = '\0';
    //每次单个字符读取响应头信息
    puts("6: 正在解析http响应头...");
    while ((len = read(client_socket, buf, 1)) != 0)
    {
        if (length + len > mem_size)
        {
            //动态内存申请, 因为无法确定响应头内容长度
            mem_size *= 2;
            char * temp = (char *) realloc(response, sizeof(char) * mem_size);
            if (temp == NULL)
            {
                printf("动态内存申请失败\n");
                exit(-1);
            }
            response = temp;
        }

        buf[len] = '\0';
        strcat(response, buf);

        //找到响应头的头部信息
        int flag = 0;
        for (int i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);
        if (flag == 4)//连续两个换行和回车表示已经到达响应头的头尾, 即将出现的就是需要下载的内容
            break;

        length += len;
    }
	printf("解析http头完毕，返回http头:\n%s\n",response);
	strHeaders = response;
	free(response);
	free(buf);
}
struct HTTP_RES_HEADER parse_header(const char *response)
{
	printf("Server response:%s",response);
    /*获取响应头的信息*/
    struct HTTP_RES_HEADER resp;

    char *pos = strstr(response, "HTTP/");
    if (pos)//获取返回代码
        sscanf(pos, "%*s %d", &resp.status_code);

    pos = strstr(response, "Content-Type:");
    if (pos)//获取返回文档类型
        sscanf(pos, "%*s %s", resp.content_type);

    pos = strstr(response, "Content-Length:");
    if (pos)//获取返回文档长度
        sscanf(pos, "%*s %lld", &resp.content_length);
	if(resp.status_code == 302)
	{
		pos = strstr(response,"Location:");
		if(pos)
			sscanf(pos,"%*s %s",resp.szLocation);	
	}
    return resp;
}

void get_ip_addr(char *host_name, char *ip_addr)
{
    /*通过域名得到相应的ip地址*/
    struct hostent *host = gethostbyname(host_name);//此函数将会访问DNS服务器
    if (!host)
    {
        ip_addr = NULL;
        return;
    }

    for (int i = 0; host->h_addr_list[i]; i++)
    {
        strcpy(ip_addr, inet_ntoa( * (struct in_addr*) host->h_addr_list[i]));
        break;
    }
}
