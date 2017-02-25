
/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string>
#include <dirent.h>
#include "UrlCode.h"
#include<log.h>
#include<map>
#include"httpCommon.h"
#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: 云守护httpd/0.1.1\r\n"

//void accept_request(int);
void * accept_request(void *thread_arg);
void bad_request(int);
void cat(int, FILE *,long long iFileOffset = 0);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void sendheaders(int,const char *,const char * pszFileType,long long iStreamLen = -1);
void not_found(int);
void serve_file(int, const char *,long long iFileLen,long long iFileOffset);
int startup(short *);
void unimplemented(int);
void list_dir_items(int,const char * pszWordDir,const char * urllink);
extern std::string getContentTypeFromFileName(const char* pszFileName);
void InitWorkHomeDir(const char * pszHomeDir);
std::string g_strWorkHomeDir;
/*A request has caused a call to accept() on the server port to
* return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
void * accept_request(void *thread_arg) {
    //子线程处理http请求
	char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    //文件描述结构体
    struct stat st;
    int cgi = 0;
    /* becomes true if server decides this is a CGI program */
    //查询参数
    char *query_string = NULL;

    long client;
    client = (long) thread_arg;
   // printf("client_sock:%d\n",client);
    //获取每行数据
    numchars = get_line(client, buf, sizeof (buf));
    i = 0;
    j = 0;
    //不是空格，有method参数
    while (!ISspace(buf[j]) && (i < sizeof (method) - 1)) {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        //方法未实现
        unimplemented(client);
        return 0;
    }

    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    i = 0;
    while (ISspace(buf[j]) && (j < sizeof (buf)))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof (url) - 1) && (j < sizeof (buf))) {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0) {
        //get方法提交数据
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?') {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }

    if (strcmp(url,"/") == 0) //根目录
	{
		chdir(g_strWorkHomeDir.c_str());
	}
	char szFullPath[1024];
	string strDeCodeUrl = UrlDecode(url);
	strcpy(url,strDeCodeUrl.c_str());
	sprintf(szFullPath,"%s%s",g_strWorkHomeDir.c_str(),url);
	WriteFormatLog(LOG_TYPE_INFO,"client header:\n%s\n",buf);
	WriteFormatLog(LOG_TYPE_INFO,"client request url =%s\n",url);
	WriteFormatLog(LOG_TYPE_INFO,"client request path =%s\n",szFullPath);
	if(stat(szFullPath,&st)==-1)
	{
		WriteFormatLog(LOG_TYPE_INFO,"%s路径无法找到",szFullPath);
		not_found(client);	
		close(client);
		return NULL;
	}
	
	if(S_ISDIR(st.st_mode))
	{
		WriteFormatLog(LOG_TYPE_INFO,"枚举目录内的文件：%s",szFullPath);
		list_dir_items(client,szFullPath,url);
		chdir(szFullPath);
	}
	else
	{
		WriteFormatLog(LOG_TYPE_INFO,"serve_file 准备调用：%s",szFullPath);
		long long iFileOffset = 0;
		serve_file(client,szFullPath,st.st_size,iFileOffset);
	}
	close(client);
	return NULL;
}

/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */

void bad_request(int client) {
    //返回400
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof (buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof (buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof (buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof (buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof (buf), 0);
}

/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */

void cat(int client, FILE *resource,long long iFileOffset) {
    //返回文件数据
    char buf[1024];	
  	if(0!=fseek(resource,iFileOffset,SEEK_SET))
	{
		WriteFormatLog(LOG_TYPE_ERROR,"fseek error:%s",strerror(errno));
	}
   	int iRead = fread(buf,sizeof(char),1024,resource); 
	do{
		if(iRead)
		{
			send(client,buf,iRead,0);
   			iRead = fread(buf,sizeof(char),1024,resource); 
		}
		
	}while(iRead >0);
	/*
	fgets(buf, sizeof (buf), resource);
    while (!feof(resource)) {
        //循环发送文件
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof (buf), resource);
    }
	*/
}

/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */

void cannot_execute(int client) {
    //返回500 服务器内部错误
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
void error_die(const char *sc) {
    perror(sc);
    exit(1);
}

/* Execute a CGI script.  Will need to set environment variables as
 * appropriate. CGI (Common Gateway Interface) 通用网关接口,参数查询
 * Parameters: client socket descriptor
 *             path to the CGI script */
void execute_cgi(int client, const char *path,
        const char *method, const char *query_string) {
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A';
    buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)
        while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
            numchars = get_line(client, buf, sizeof (buf));
    else /* POST */ {
        numchars = get_line(client, buf, sizeof (buf));
        while ((numchars > 0) && strcmp("\n", buf)) {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof (buf));
        }
        if (content_length == -1) {
            bad_request(client);
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    //创建管道
    if (pipe(cgi_output) < 0) {
        //返回500 服务器内部错误
        cannot_execute(client);
        return;
    }
    if (pipe(cgi_input) < 0) {
        cannot_execute(client);
        return;
    }

    if ((pid = fork()) < 0) {
        cannot_execute(client);
        return;
    }
    if (pid == 0) /* child: CGI script */ {
        //再子进程中处理哦
        char meth_env[255];
        char query_env[255];
        char length_env[255];
        //操作管道的读写端
        dup2(cgi_output[1], 1);
        dup2(cgi_input[0], 0);
        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        //放入环境变量
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        } else { /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        //执行path
        execl(path, path, NULL);
        exit(0);
    } else { /* parent */
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++) {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}

/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */

int get_line(int sock, char *buf, int size) {
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n')) {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        } else
            c = '\n';
    }
    buf[i] = '\0';

    return (i);
}


/** Return the informational HTTP headers about a file. */

/* Parameters: the socket to print the headers on
 *             the name of the file */

void sendheaders(int client, const char *filename,const char *pszFileType,long long iStreamLen) {
    //先返回文件头部信息
    char buf[1024];
    (void) filename; /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: %s\r\n",pszFileType);
    send(client, buf, strlen(buf), 0);
	std::string strTypeStream = "application/octet-stream";
/*	if(strcmp(pszFileType,strTypeStream.c_str())==0)
	{*/
		sprintf(buf,"Content-Length: %lld\r\n",iStreamLen);
		send(client,buf,strlen(buf),0);
		strcpy(buf,"Accept-Ranges: bytes\r\n");
		send(client,buf,strlen(buf),0);
		strcpy(buf,"ETag: \"2f38a6cac7cec51:160c\"\r\n");
		send(client,buf,strlen(buf),0);
	//}
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}
/* Give a client a 404 not found status message. */

void not_found(int client) {
    //返回http 404协议
    char buf[1024];
    //先发送http协议头
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    //再发送serverName
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    //再发送Content-Type
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    //发送换行符
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    //发送html主体内容
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */

void serve_file(int client, const char *filename,long long iFileLen,long long iFileOffset) {
    //返回文件数据
    printf("serve_file input data is[%s],iFileLen=%lld,iFileOffset=%lld\n",filename,iFileLen,iFileOffset);
	    //返回文件数据
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A';
    buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
        numchars = get_line(client, buf, sizeof (buf));

    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else {
        //先返回头部信息
		std::string strType = getContentTypeFromFileName(filename);
		WriteFormatLog(LOG_TYPE_INFO,"file [%s] type return[%s]\n",filename,strType.c_str());
		sendheaders(client, filename,strType.c_str(),iFileLen);
		WriteFormatLog(LOG_TYPE_INFO,"开始发送文件数据...");
		cat(client, resource,iFileOffset);
		WriteFormatLog(LOG_TYPE_INFO,"文件数据发送完毕!");
    }
    fclose(resource);
}

/** This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
int startup(short *port) {
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof (name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(httpd, (struct sockaddr *) &name, sizeof (name)) < 0)
        error_die("bind");
    if (*port == 0) /* if dynamically allocating a port */ {
        int namelen = sizeof (name);
        if (getsockname(httpd, (struct sockaddr *) &name, (socklen_t*)&namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    if (listen(httpd, 5) < 0)
        error_die("listen");
    return (httpd);
}

/** Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */

void unimplemented(int client) {
    //方法未实现
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

void PrintSocketAddress(const struct sockaddr *address, FILE *stream) {
    // Test for address and stream
    if (address == NULL || stream == NULL)
        return;

    void *numericAddress; // Pointer to binary address
    // Buffer to contain result (IPv6 sufficient to hold IPv4)
    char addrBuffer[INET6_ADDRSTRLEN];
    in_port_t port; // Port to print
    // Set pointer to address based on address family
    switch (address->sa_family) {
        case AF_INET:
            numericAddress = &((struct sockaddr_in *) address)->sin_addr;
            port = ntohs(((struct sockaddr_in *) address)->sin_port);
            break;
        case AF_INET6:
            numericAddress = &((struct sockaddr_in6 *) address)->sin6_addr;
            port = ntohs(((struct sockaddr_in6 *) address)->sin6_port);
            break;
        default:
            fputs("[unknown type]", stream); // Unhandled type
            return;
    }
    // Convert binary to printable address
    if (inet_ntop(address->sa_family, numericAddress, addrBuffer,
            sizeof (addrBuffer)) == NULL)
        fputs("[invalid address]", stream); // Unable to convert
    else {
        fprintf(stream, "来自%s", addrBuffer);
        if (port != 0) // Zero not valid in any socket addr
            fprintf(stream, "-%u\n", port);
    }
}

void list_dir_items(int client,const char * pszWorkDir,const char * url)
{
	sendheaders(client,url,"text/html");
	struct dirent * entry = NULL;
	DIR * pDir = NULL;
    struct stat statbuf;
	std::string strData = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/></head>";
	send(client,strData.c_str(),strData.length(),0);
	strData = "<body>";
	send(client,strData.c_str(),strData.length(),0);
	pDir = opendir(pszWorkDir);
	while(NULL!=(entry = readdir(pDir)))
	{
	  char szFullPath[1024];
	  sprintf(szFullPath,"%s%s",pszWorkDir,entry->d_name);
	  printf("get one item [%s]\n",szFullPath);
      lstat(szFullPath,&statbuf);
      if( S_ISDIR(statbuf.st_mode) )
      {
		  if(strcmp(".",entry->d_name)==0 || strcmp("..",entry->d_name)==0)
          continue;

        char szBuffer[2048];
		sprintf(szBuffer,"&nbsp;<A href=\"%s%s/\">%s</A>&nbsp;&nbsp;[DIR]<br/>"
		,url,entry->d_name, entry->d_name);
		send(client,szBuffer,strlen(szBuffer),0);
      }
      else
      {
            char szBuffer[2048];
			sprintf(szBuffer,"&nbsp;<A href=\"%s%s\">%s</A>&nbsp;&nbsp;%lld&nbsp;bytes<br/>"
			,url,entry->d_name,entry->d_name,statbuf.st_size );
			send(client,szBuffer,strlen(szBuffer),0);
      }
	}
	strData = "</body></html>";
	send(client,strData.c_str(),strData.length(),0);
}
void InitWorkHomeDir(const char * pszHomeDir)
{
	g_strWorkHomeDir = pszHomeDir;
	chdir(g_strWorkHomeDir.c_str());
}
