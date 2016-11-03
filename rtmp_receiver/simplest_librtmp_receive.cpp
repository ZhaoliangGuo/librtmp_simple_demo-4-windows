/**
 * Simplest Librtmp Receive
 *
 * 雷霄骅，张晖
 * leixiaohua1020@126.com
 * zhanghuicuc@gmail.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序用于接收RTMP流媒体并在本地保存成FLV格式的文件。
 * This program can receive rtmp live stream and save it as local flv file.
 */
#include <stdio.h>
#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"
#include <time.h>

// Modified by ZL Guo 20161103

int InitSockets()
{
#ifdef WIN32
	WORD version;
	WSADATA wsaData;
	version = MAKEWORD(1, 1);
	return (WSAStartup(version, &wsaData) == 0);
#endif
}

void CleanupSockets()
{
#ifdef WIN32
	WSACleanup();
#endif
}

int main(int argc, char* argv[])
{
	InitSockets();
	
	int nRead;

	bool bLiveStream = true;				
	
	int nBufSize   = 1024*1024*10;
	char *szBuffer = (char*)malloc(nBufSize);
	memset(szBuffer, 0, nBufSize);

	long nCountBufSize = 0;

	char szAppPath[MAX_PATH] = {0};
	DWORD dwRet = GetModuleFileNameA(NULL, szAppPath, MAX_PATH);
	if (dwRet == MAX_PATH)
	{
		strcpy(szAppPath, ".");
	}

	(strrchr(szAppPath,'\\'))[0] = '\0';

	SYSTEMTIME sSystemTime;
	GetLocalTime(&sSystemTime);

	char szFileName[_MAX_PATH] = {0};
	sprintf_s(szFileName,
		_MAX_PATH, 
		"%s\\rtmp_stream_%4d%02d%02d%02d%02d%02d.flv",
		szAppPath,
		sSystemTime.wYear, 
		sSystemTime.wMonth, 
		sSystemTime.wDay,
		sSystemTime.wHour,
		sSystemTime.wSecond,
		sSystemTime.wSecond);

	FILE *fp=fopen(szFileName, "wb");
	if (!fp)
	{
		RTMP_LogPrintf("Open File Error.\n");
		CleanupSockets();

		return -1;
	}
	
	/* set log level */
	//RTMP_LogLevel loglvl=RTMP_LOGDEBUG;
	//RTMP_LogSetLevel(loglvl);

	RTMP *rtmp = RTMP_Alloc();
	RTMP_Init(rtmp);

	//set connection timeout,default 30s
	rtmp->Link.timeout = 10;

	char szRTMP_URL[_MAX_PATH] = {0};
	sprintf_s(szRTMP_URL, 
			  _MAX_PATH, 
			  "%s", 
			  "rtmp://live.hkstv.hk.lxdns.com/live/hks");
			  //"rtmp://localhost:1935/vod/mp4:sample.mp4");
			  //"rtmp://192.168.2.109:1935/vod/mp4:sample.mp4");

	// HKS's live URL
	if(!RTMP_SetupURL(rtmp, szRTMP_URL))
	{
		RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
		RTMP_Free(rtmp);
		CleanupSockets();

		return -1;
	}

	if (bLiveStream)
	{
		rtmp->Link.lFlags|=RTMP_LF_LIVE;
	}
	
	//1hour
	RTMP_SetBufferMS(rtmp, 3600*1000);		
	
	if(!RTMP_Connect(rtmp,NULL))
	{
		RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
		RTMP_Free(rtmp);
		CleanupSockets();

		return -1;
	}

	if(!RTMP_ConnectStream(rtmp,0))
	{
		RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		CleanupSockets();

		return -1;
	}

	int i = 0;

	time_t tmStartTime;
	time_t tmEndTime;

	tmStartTime = time(NULL);

	while(nRead = RTMP_Read(rtmp, szBuffer, nBufSize))
	{
		fwrite(szBuffer, 1, nRead, fp);
		nCountBufSize += nRead;

		i++;

		RTMP_LogPrintf("%04d: Receive: %5d Bytes, Total: %5.2f KB\n", i, nRead, nCountBufSize*1.0/1024);

		if (i == 1000)
		{
			tmEndTime = time(NULL);

			break;
		}	
	}

	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}
		
	if(szBuffer)
	{
		free(szBuffer);
		szBuffer = NULL;
	}

	if(rtmp)
	{
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		CleanupSockets();

		rtmp = NULL;
	}

	RTMP_LogPrintf("FileName: %s.\n Total: %5.2f MB, UseTime: %ld s\n", 
		szFileName, 
		nCountBufSize*1.0/(1024*1024),
		tmEndTime - tmStartTime);

	getchar();

	return 0;
}