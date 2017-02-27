#include<string>
using namespace std;
#define stricmp strcasecmp
std::string getContentTypeFromFileName(const char* pszFileName)
{
	std::string strType = "application/octet-stream";

	const char *pExt = strrchr(pszFileName, '.');
	if(pExt && strlen(pExt) < 19)
	{
		char szExt[20];
		strcpy(szExt, pExt + 1);

		if(stricmp(szExt, "jpg") == 0)
		{
			strType =  "image/jpeg";
		}
		else if(stricmp(szExt, "txt") == 0
		|| stricmp(szExt,"cpp")==0
		|| stricmp(szExt,"c")==0
		|| stricmp(szExt,"rgs")==0
		|| stricmp(szExt,"def")==0
		|| stricmp(szExt,"h")==0
		|| stricmp(szExt,"bat")==0
		|| stricmp(szExt,"sh")==0
		|| stricmp(szExt,"hpp")==0
		|| stricmp(szExt,"cxx")==0
		|| stricmp(szExt,"xml")==0
		|| stricmp(szExt,"js")==0
		||stricmp(szExt,"cs")==0
		||stricmp(szExt,"log")==0
		|| stricmp(szExt,"ini")==0
		|| stricmp(szExt,"pro")==0
		|| stricmp(szExt,"nsi")==0
		||stricmp(szExt,"nsh")==0)
		{
			strType = "text/plain";
		}
		else if(stricmp(szExt, "htm") == 0)
		{
			strType = "text/html";
		}
		else if(stricmp(szExt, "html") == 0)
		{
			strType = "text/html";
		}
		else if(stricmp(szExt, "gif") == 0)
		{
			strType = "image/gif";
		}
		else if(stricmp(szExt, "png") == 0)
		{
			strType = "image/png";
		}
		else if(stricmp(szExt, "bmp") == 0)
		{
			strType = "image/x-xbitmap";
		}
		else
		{
		}
	}

	return strType;
}
