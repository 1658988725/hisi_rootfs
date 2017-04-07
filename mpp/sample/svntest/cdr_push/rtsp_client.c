/************************************************************************	
** Filename: 	rtsp_client.c
** Description:  
** Author: 	xjl
** Create Date: 2016-12-7
** Version: 	v1.0
   Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>
*************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h> 
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <linux/sockios.h>

#include"base64.h"
#include "sample_comm.h"

#include"md5.h"

#define RTSP_ACK_OK  "RTSP/1.0 200 OK"
#define OVER_TIME  3
#define CRLF    "\r\n"
#define SOCKET_ERROR -1
#define XDEBUG_RTSP_MESSAGE 1

typedef struct{
	int sendVal;
	char IPC_url[120];
	char sendBuf[1500];
	char recvBuf[3500];
}VdecRtspParam;

int g_iSeqID = 0; 
int g_iUDPSeverPortOfRtpSession = 0;
char cSessionBuf[64] = {0};

extern int g_RtspSessionTcpSock;
extern char cRtspSeverIP[16];

/*digest ralation*/
unsigned char ucUserName[20] = {0};
unsigned char ucPasswd[20] = {0};
static unsigned char ucRealm[40] = {0};
static unsigned char ucNonce[40] = {0};
static unsigned char ucResponse[40] = {0};

static unsigned char ucDigestFlag = 1;

extern char rtp_over_udp;

extern char easy_push_flag;
/*
基本认证（basic authentication）和摘要认证（ digest authentication　）
RTSP客户端应该使用username + password并计算response如下:
(1)当password为MD5编码,则
   response = md5(password:nonce:md5(public_method:url));
(2)当password为ANSI字符串,则
    response= md5(md5(username:realm:password):nonce:md5(public_method:url));
客户端在每次发起不同的请求方法时都需要计算response字段，同样在服务器端校验时也默认采取同样的计算方法
 
*/
static int SetResponse(const char *method,const char *url)
{
    //1 md5(username:realm:password)
    char buff1[200] = {0};      
    sprintf((char *)buff1,"%s:%s:%s",ucUserName,ucRealm,ucPasswd);        
    unsigned char decrypt1[16] = {0};      
    MD5_CTX md5_v1;  
    MD5Init(&md5_v1);                
    MD5Update(&md5_v1,(unsigned char *)buff1,strlen((char *)buff1));  
    MD5Final(&md5_v1,decrypt1);

    unsigned char ucDecrypt1[33] = {0};
    sprintf((char *)ucDecrypt1,
    (char *)"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
    decrypt1[0],decrypt1[1],decrypt1[2],decrypt1[3],
    decrypt1[4],decrypt1[5],decrypt1[6],decrypt1[7],
    decrypt1[8],decrypt1[9],decrypt1[10],decrypt1[11],
    decrypt1[12],decrypt1[13],decrypt1[14],decrypt1[15]   
    );

    //3 md5(public_method:url)
    char buff3[200] = {0};   
    sprintf((char *)buff3,"%s:%s",method,url);
    unsigned char decrypt3[16] = {0};      
    MD5_CTX md5_v3;  
    MD5Init(&md5_v3);                
    MD5Update(&md5_v3,(unsigned char *)buff3,strlen((char *)buff3));  
    MD5Final(&md5_v3,decrypt3);  

    unsigned char ucDecrypt3[33] = {0};
    sprintf((char *)ucDecrypt3,
    "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
    decrypt3[0],decrypt3[1],decrypt3[2],decrypt3[3],
    decrypt3[4],decrypt3[5],decrypt3[6],decrypt3[7],
    decrypt3[8],decrypt3[9],decrypt3[10],decrypt3[11],
    decrypt3[12],decrypt3[13],decrypt3[14],decrypt3[15] 
    );

    //4 md5(md5(username:realm:password):nonce:md5(public_method:url))
    char buff4[200] = {0};  
    sprintf((char *)buff4,"%s:%s:%s",ucDecrypt1,ucNonce,ucDecrypt3);
    unsigned char decrypt4[16] = {0};      
    MD5_CTX md5_v4;  
    MD5Init(&md5_v4);                
    MD5Update(&md5_v4,(unsigned char *)buff4,strlen((char *)buff4));  
    MD5Final(&md5_v4,decrypt4); 

    unsigned char ucDecrypt4[33] = {0};
    sprintf((char *)ucDecrypt4,
    "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
    decrypt4[0],decrypt4[1],decrypt4[2],decrypt4[3],
    decrypt4[4],decrypt4[5],decrypt4[6],decrypt4[7],
    decrypt4[8],decrypt4[9],decrypt4[10],decrypt4[11],
    decrypt4[12],decrypt4[13],decrypt4[14],decrypt4[15]
    );
    memcpy(ucResponse,ucDecrypt4,strlen((char*)ucDecrypt4));
    printf("ucResponse:%s\n",ucResponse);
    
    return 0;
}


int SendOpationRequest(const char *cmd_url)
{
    VdecRtspParam sRtspSendParam;
    char str[500] = {0};
    int res = 0;
   
    //g_iSeqID = 0;  
    g_iSeqID = g_iSeqID + 1;

    sprintf(str,
        "OPTIONS %s RTSP/1.0"CRLF
        "CSeq: %d"CRLF
        "User-Agent: Lavf57.28.100"CRLF
        CRLF,cmd_url,g_iSeqID
    );  
    printf("Sent rtsp request:\n%s strlen :%d\n", str,strlen(str));
    send(g_RtspSessionTcpSock,str, strlen(str), MSG_NOSIGNAL);	   
    res = recv(g_RtspSessionTcpSock,sRtspSendParam.recvBuf,sizeof(sRtspSendParam.recvBuf),0);
    if(res < 0){
       printf("do OPTIONS error \n");
       return 0;
    }
    printf("OPTIONS.recvBuf\n%s\n",sRtspSendParam.recvBuf);  
    if(strstr(sRtspSendParam.recvBuf,"RTSP/1.0 200 OK")!=NULL){
       printf("set OPTIONS ok\n");
       return 1;
    } 

    return 1;
}

/*
sprop-parameter-sets=Z2QAHqzZQKA9oQAAAwABAAADADIPFi2W,aOvjyyLA
sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvjyyLA; profile-level-id=64001E
sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvjyyLA; profile-level-id=64001E
sprop-parameter-sets=Z2QAFazZQIAlsBEAAAMAAQAAAwA8DxYtlg==,aOvssiw=; profile-level-id=640015

sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvssiw=; profile-level-id=64001E
sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvssiw=; profile-level-id=64001E
sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvssiw=; profile-level-id=64001E
this is sps base64
"Content-Length: 493 "CRLF  +4

s=Big Buck Bunny, Sunflower version
*/
int DoAnnounce1(const char *cURL)
{	 
    int i = 0;
    VdecRtspParam sRtspSendParam; 	    
	sRtspSendParam.sendVal = 0;
	memset(sRtspSendParam.sendBuf,0,sizeof(sRtspSendParam.sendBuf));
	memset(sRtspSendParam.recvBuf,0,sizeof(sRtspSendParam.recvBuf));	
	memset(sRtspSendParam.IPC_url,0,sizeof(sRtspSendParam.IPC_url));
    
    g_iSeqID = g_iSeqID+1;

    memset(sRtspSendParam.sendBuf,0,sizeof(sRtspSendParam.sendBuf));           
#if(1)
    sprintf(sRtspSendParam.sendBuf,        
        "ANNOUNCE %s RTSP/1.0"CRLF
        "Content-Type: application/sdp"CRLF
        "CSeq: %d"CRLF
        "User-Agent: Lavf57.28.100"CRLF
        "Content-Length: 523"CRLF/*497*/
        CRLF
        "v=0"CRLF
        "o=- 0 0 IN IP4 127.0.0.1"CRLF
        /*"s=No Name"CRLF*/
        "s=Big Buck Bunny, Sunflower version"CRLF
        "c=IN IP4 %s"CRLF
        "t=0 0"CRLF
        "a=tool:libavformat 57.28.100"CRLF
        "m=video 0 RTP/AVP 96"CRLF
        "a=rtpmap:96 H264/90000"CRLF
        /*"a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvjyyLA; profile-level-id=64001E"CRLF*/
        "a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvssiw=; profile-level-id=64001E"CRLF        
        "a=control:streamid=0"CRLF
        "m=audio 0 RTP/AVP 97"CRLF
        "b=AS:128"CRLF
        "a=rtpmap:97 MPEG4-GENERIC/44100/2"CRLF
        "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=121056E500"CRLF
        "a=control:streamid=1"CRLF
        CRLF,cURL,g_iSeqID,cRtspSeverIP
    );
#else
    sprintf(sRtspSendParam.sendBuf,        
        "ANNOUNCE %s RTSP/1.0"CRLF
        "Content-Type: application/sdp"CRLF
        "CSeq: %d"CRLF
        "User-Agent: Lavf57.28.100"CRLF
        "Content-Length: 335"CRLF/*497*/
        CRLF
        "v=0"CRLF
        "o=- 0 0 IN IP4 127.0.0.1"CRLF
        /*"s=No Name"CRLF*/
        "s=Big Buck Bunny, Sunflower version"CRLF
        "c=IN IP4 %s"CRLF
        "t=0 0"CRLF
        "a=tool:libavformat 57.28.100"CRLF
        "m=video 0 RTP/AVP 96"CRLF
        "a=rtpmap:96 H264/90000"CRLF
        "a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvssiw=; profile-level-id=64001E"CRLF
        "a=control:streamid=0"CRLF
        CRLF,cURL,g_iSeqID,cRtspSeverIP
    );
#endif
   memset(sRtspSendParam.recvBuf,0,sizeof(sRtspSendParam.recvBuf));	   
   printf("Sent ANNOUNCE1 request:\n%sstrlen:%d\n", sRtspSendParam.sendBuf,strlen(sRtspSendParam.sendBuf));   
   sRtspSendParam.sendVal = send(g_RtspSessionTcpSock,sRtspSendParam.sendBuf, strlen(sRtspSendParam.sendBuf), MSG_NOSIGNAL);	      
   if (SOCKET_ERROR == sRtspSendParam.sendVal)
   {
	 printf("send ANNOUNCE1 failed! %d \n",errno) ;
	 return 0;
   }
   int res = recv(g_RtspSessionTcpSock,sRtspSendParam.recvBuf,sizeof(sRtspSendParam.recvBuf),0);
   printf("ANNOUNCE1 %s",sRtspSendParam.recvBuf); 
   if(res == -1){
       printf("do ANNOUNCE1 error \n");
       return 0;
    }
    
    if(strstr(sRtspSendParam.recvBuf,"RTSP/1.0 200 OK")!=NULL){
       printf("set ANNOUNCE1 ok\n");
    }   

  /*get Authenticate ralation*/
    char *pDst1=strstr(sRtspSendParam.recvBuf,"WWW-Authenticate: ");
    if(pDst1 !=NULL){
      char *pDst2=strstr(pDst1,"realm=");
      pDst2+=7; 
      
      printf("realm: ");
      for(i=0;i<20;i++)
      {
         if(*pDst2=='"')break;
         ucRealm[i] = *pDst2++;         
         printf("%c",ucRealm[i]);//限长20
      }
      ucRealm[i] = '\0';
      printf("\n");
                   
      char *pDst3 = strstr(pDst2,"nonce=");
      if(pDst3!=NULL){         
            pDst3+=7;
            i=0;
            printf("ucNonce: ");
            for(i=0;i<35;i++)//32
            {
             if(*pDst3=='"')break;
             ucNonce[i] = *pDst3++;  
             printf("%c",ucNonce[i]);
            }
            ucNonce[i] = '\0';
            printf("\n");
      }else{
         printf("get ucNonce fails!\n");
         return -1;
      }    
      ucDigestFlag = 1;
    }else{
      char *pDstNoDigest = strstr(sRtspSendParam.recvBuf,RTSP_ACK_OK);
      if(pDstNoDigest!=NULL){
         ucDigestFlag = 0;
         printf("The streaming media server does not require authentication\n");
      }
    }

    if(easy_push_flag == 0x00){
        char* pDst = strstr(sRtspSendParam.recvBuf, "Session: ");
        if(pDst!=NULL){
           sscanf(pDst, "Session: %[^;]", cSessionBuf);     
           printf("Session ID:%s\n",cSessionBuf);
        }else{
           printf("get session fails\n");
           return -1;
        }
    }
  return 1;         
}


int DoAnnounce2(const char *cURL)
{	 
    if(ucDigestFlag == 0x00) return 1;

	VdecRtspParam sRtspSendParam; 	
	sRtspSendParam.sendVal = 0;
	memset(sRtspSendParam.sendBuf,0,sizeof(sRtspSendParam.sendBuf));
	memset(sRtspSendParam.recvBuf,0,sizeof(sRtspSendParam.recvBuf));	
	memset(sRtspSendParam.IPC_url,0,sizeof(sRtspSendParam.IPC_url));

    g_iSeqID = g_iSeqID+1;
    
    memset(sRtspSendParam.sendBuf,0,sizeof(sRtspSendParam.sendBuf));
    SetResponse("ANNOUNCE",cURL); 

    #if(1)    
    sprintf(sRtspSendParam.sendBuf,        
        "ANNOUNCE %s RTSP/1.0"CRLF
        "Content-Type: application/sdp"CRLF
        "CSeq: %d"CRLF
        "User-Agent: Lavf57.28.100"CRLF
        "Session: %s"CRLF
        "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\""CRLF
        "Content-Length: 523"CRLF
        CRLF
        "v=0"CRLF
        "o=- 0 0 IN IP4 127.0.0.1"CRLF
        /*"s=No Name"CRLF*/
        "s=Big Buck Bunny, Sunflower version"CRLF
        "c=IN IP4 %s"CRLF
        "t=0 0"CRLF
        "a=tool:libavformat 57.28.100"CRLF
        "m=video 0 RTP/AVP 96"CRLF
        "a=rtpmap:96 H264/90000"CRLF
        /*"a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvjyyLA; profile-level-id=64001E"CRLF*/
        "a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvssiw=; profile-level-id=64001E"CRLF                
        "a=control:streamid=0"CRLF
        "m=audio 0 RTP/AVP 97"CRLF
        "b=AS:128"CRLF
        "a=rtpmap:97 MPEG4-GENERIC/44100/2"CRLF
        "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=121056E500"CRLF
        "a=control:streamid=1"CRLF
        CRLF,cURL,g_iSeqID,
        cSessionBuf,ucUserName,ucRealm,ucNonce,cURL,ucResponse,cRtspSeverIP
    );
    #else
     sprintf(sRtspSendParam.sendBuf,        
        "ANNOUNCE %s RTSP/1.0"CRLF
        "Content-Type: application/sdp"CRLF
        "CSeq: %d"CRLF
        "User-Agent: Lavf57.28.100"CRLF
        "Session: %s"CRLF
        "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\""CRLF
        "Content-Length: 335"CRLF
        CRLF
        "v=0"CRLF
        "o=- 0 0 IN IP4 127.0.0.1"CRLF
        /*"s=No Name"CRLF*/
        "s=Big Buck Bunny, Sunflower version"CRLF
        "c=IN IP4 %s"CRLF
        "t=0 0"CRLF
        "a=tool:libavformat 57.28.100"CRLF
        "m=video 0 RTP/AVP 96"CRLF
        "a=rtpmap:96 H264/90000"CRLF
        "a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAHqzZQKA9psgAAAMACAAAAwGQeLFssA==,aOvssiw=; profile-level-id=64001E"CRLF
        "a=control:streamid=0"CRLF
        CRLF,cURL,g_iSeqID,
        cSessionBuf,ucUserName,ucRealm,ucNonce,cURL,ucResponse,cRtspSeverIP
    );
    #endif
   
   memset(sRtspSendParam.recvBuf,0,sizeof(sRtspSendParam.recvBuf));	   
   printf("Sent ANNOUNCE2 request:\n%sstrlen:%d\n", sRtspSendParam.sendBuf,strlen(sRtspSendParam.sendBuf));   
   sRtspSendParam.sendVal = send(g_RtspSessionTcpSock,sRtspSendParam.sendBuf, strlen(sRtspSendParam.sendBuf), MSG_NOSIGNAL);	   
   if (SOCKET_ERROR == sRtspSendParam.sendVal)
   {
	 printf("send ANNOUNCE2 failed! %d \n",errno) ;
	 return 0;
   }

    int res = recv(g_RtspSessionTcpSock,sRtspSendParam.recvBuf,sizeof(sRtspSendParam.recvBuf),0);
    printf("ANNOUNCE2 \n%s",sRtspSendParam.recvBuf); 
    if(res == -1){
       printf("do ANNOUNCE2 error \n");
       return 0;
    }    
    if(strstr(sRtspSendParam.recvBuf,"RTSP/1.0 200 OK")!=NULL){
       printf("set ANNOUNCE2 ok\n");
       return 1;
    }
  return 1;     
}

int DoSetUp(const char *cURL)
{		   
    VdecRtspParam sRtspSendParam;
    memset(sRtspSendParam.sendBuf,0,sizeof(sRtspSendParam.sendBuf));
    memset(sRtspSendParam.recvBuf,0,sizeof(sRtspSendParam.recvBuf));

    g_iSeqID = g_iSeqID+1;


    if(rtp_over_udp == 0x01){
        if(ucDigestFlag == 0x00){
            if(easy_push_flag == 0x00){
                sprintf(sRtspSendParam.sendBuf,
                    "SETUP %s/streamid=0 RTSP/1.0"CRLF        
                    "CSeq: %d"CRLF
                    "User-Agent: Lavf57.28.100"CRLF
                    "Transport: RTP/AVP/UDP;unicast;client_port=25174-25175;mode=record"CRLF
                    "Session: %s"CRLF
                    CRLF,cURL,g_iSeqID,cSessionBuf
                );
            }else{
                sprintf(sRtspSendParam.sendBuf,
                    "SETUP %s/streamid=0 RTSP/1.0"CRLF        
                    "CSeq: %d"CRLF
                    "User-Agent: Lavf57.28.100"CRLF
                    "Transport: RTP/AVP/UDP;unicast;client_port=25174-25175;mode=record"CRLF
                    CRLF,cURL,g_iSeqID
                );
            }
        }else{
            SetResponse("SETUP",cURL);
            sprintf(sRtspSendParam.sendBuf,
                "SETUP %s/streamid=0 RTSP/1.0"CRLF        
                "CSeq: %d"CRLF
                "User-Agent: Lavf57.28.100"CRLF
                "Transport: RTP/AVP/UDP;unicast;client_port=25174-25175;mode=record"CRLF
                "Session: %s"CRLF
                "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\""CRLF		
                CRLF,cURL,g_iSeqID,cSessionBuf,
                ucUserName,ucRealm,ucNonce,cURL,ucResponse
            );
        }
    }else{
      if(ucDigestFlag == 0x00){
           if(easy_push_flag == 0x01){
                sprintf(sRtspSendParam.sendBuf,
                    "SETUP %s/streamid=0 RTSP/1.0"CRLF        
                    "CSeq: %d"CRLF
                    "User-Agent: Lavf57.28.100"CRLF
                    //"Transport: RTP/AVP/UDP;unicast;client_port=25174-25175;mode=record"CRLF
                    "Transport: RTP/AVP/TCP;unicast;interleaved=0-1;mode=record"CRLF
                    CRLF,cURL,g_iSeqID
                );
            }else{
                sprintf(sRtspSendParam.sendBuf,
                    "SETUP %s/streamid=0 RTSP/1.0"CRLF        
                    "CSeq: %d"CRLF
                    "User-Agent: Lavf57.28.100"CRLF
                    //"Transport: RTP/AVP/UDP;unicast;client_port=25174-25175;mode=record"CRLF
                    "Transport: RTP/AVP/TCP;unicast;interleaved=0-1;mode=record"CRLF
                    "Session: %s"CRLF
                    CRLF,cURL,g_iSeqID,cSessionBuf
                );
            }
        }else{
            SetResponse("SETUP",cURL);
            sprintf(sRtspSendParam.sendBuf,
                "SETUP %s/streamid=0 RTSP/1.0"CRLF        
                "CSeq: %d"CRLF
                "User-Agent: Lavf57.28.100"CRLF
                //"Transport: RTP/AVP/UDP;unicast;client_port=25174-25175;mode=record"CRLF
                "Transport: RTP/AVP/TCP;unicast;interleaved=0-1;mode=record"CRLF
                "Session: %s"CRLF
                "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\""CRLF		
                CRLF,cURL,g_iSeqID,cSessionBuf,
                ucUserName,ucRealm,ucNonce,cURL,ucResponse
            );
        }
    }

    printf("Sent SETUP request:\n%sstrlen:%d\n", sRtspSendParam.sendBuf,strlen(sRtspSendParam.sendBuf));
    sRtspSendParam.sendVal = send(g_RtspSessionTcpSock, sRtspSendParam.sendBuf, strlen(sRtspSendParam.sendBuf), MSG_NOSIGNAL);
    if (SOCKET_ERROR == sRtspSendParam.sendVal){
         printf("send SETUP failed! \n") ;
         return 0;
    }

    int res = recv(g_RtspSessionTcpSock,sRtspSendParam.recvBuf,sizeof(sRtspSendParam.recvBuf),0);
    printf("SETUP.recvBuf\n%s\n",sRtspSendParam.recvBuf);
    if(res == -1){
      printf("Do SETUP error \n");
      return 0;
    }

    if(strstr(sRtspSendParam.recvBuf,"RTSP/1.0 200 OK")!=NULL){
       printf("set up ok\n");
       //return 1;
    }


    if(easy_push_flag == 0x01){
        char* pDst = strstr(sRtspSendParam.recvBuf, "Session: ");
        if(pDst!=NULL){
          sscanf(pDst, "Session: %[^\r]", cSessionBuf);     
          printf("Session ID:%s\n",cSessionBuf);
        }else{
          printf("get session fails\n");
         return -1;
        }
    }

    if(rtp_over_udp == 0x00){
        return 1;//tcp
    }
    
    /*to get  udp server port of rtp session*/
    char cPort[10] = {0}; 
    char* pDst2 = strstr(sRtspSendParam.recvBuf, "Transport: ");
    if(pDst2==NULL) return -1;
    //printf("pDst2:\n%s\n",pDst2);    
    
    char* pDst3 = strstr(pDst2, "server_port=");
    if(pDst3==NULL) return -1;
    
    char* pDst4 = strstr(pDst3, "=");
    if(pDst4==NULL) return -1;    
    //printf("pDst4:\n%s\n",pDst4);
    int i = 0;
    while (*(++pDst4)!='-')
    {
        *(cPort+i)=*pDst4;
        ++i;
    }
    //printf("cPort:%s\n",cPort);
    g_iUDPSeverPortOfRtpSession = atoi(cPort);
    printf("port:%d\n",atoi(cPort));


    //printf("DoSetUp %d\n",g_iSeqID);
    return 1;
}


int DoRecord(const char *cURL)
{	
	VdecRtspParam sRtspSendParam;	
	memset(sRtspSendParam.sendBuf,0,sizeof(sRtspSendParam.sendBuf));
	memset(sRtspSendParam.recvBuf,0,sizeof(sRtspSendParam.recvBuf));		
    g_iSeqID = g_iSeqID + 1;
    if(ucDigestFlag==0x00){
     	sprintf(sRtspSendParam.sendBuf,
            "RECORD %s RTSP/1.0"CRLF
            "Range: npt=0.000-"CRLF
            "CSeq: %d"CRLF
            "User-Agent: Lavf57.28.100"CRLF
            "Session: %s"CRLF
            CRLF,cURL,g_iSeqID,cSessionBuf
        );
    }else{
        SetResponse("RECORD",cURL);
    	   sprintf(sRtspSendParam.sendBuf,
            "RECORD %s RTSP/1.0"CRLF
            "Range: npt=0.000-"CRLF
            "CSeq: %d"CRLF
            "User-Agent: Lavf57.28.100"CRLF
            "Session: %s"CRLF
            "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\""CRLF
            CRLF,cURL,g_iSeqID,cSessionBuf,
            ucUserName,ucRealm,ucNonce,cURL,ucResponse
        ); 
    }
    
    printf("Sent RECORD request:\n%s strlen:%d\n", sRtspSendParam.sendBuf,strlen(sRtspSendParam.sendBuf));
	sRtspSendParam.sendVal = send(g_RtspSessionTcpSock, sRtspSendParam.sendBuf, strlen(sRtspSendParam.sendBuf), MSG_NOSIGNAL);
	if (SOCKET_ERROR == sRtspSendParam.sendVal){
        	 printf("send RECORD failed!\n") ;
        	 return 0;
	}
	int res = recv(g_RtspSessionTcpSock,sRtspSendParam.recvBuf,sizeof(sRtspSendParam.recvBuf),0);
    printf("RECORD.recvBuf\n%s\n",sRtspSendParam.recvBuf);
	if(res == -1){
       printf("Do RECORD error \n");
	   return -1;
	}
    if(strstr(sRtspSendParam.recvBuf,"RTSP/1.0 200 OK")!=NULL){
       printf("连接成功\n");
       return 1;
    }

   return 0;
}


/*
TEARDOWN rtsp://192.168.20.136:5000/xxx666 RTSP/1.0 
CSeq: 5 
Session: 6310936469860791894 
User-Agent: VLC media player (LIVE555 Streaming Media v2005.11.10)
*/
int DoTearDown(const char *cURL)
{	
	VdecRtspParam sRtspSendParam;	
	memset(sRtspSendParam.sendBuf,0,sizeof(sRtspSendParam.sendBuf));
	memset(sRtspSendParam.recvBuf,0,sizeof(sRtspSendParam.recvBuf));
    
	g_iSeqID = g_iSeqID + 1;

    if(ucDigestFlag==0x00){
     	sprintf(sRtspSendParam.sendBuf,
            "TEARDOWN %s RTSP/1.0"CRLF
            "CSeq: %d"CRLF
            "User-Agent: Lavf57.28.100"CRLF
            "Session: %s"CRLF
            CRLF,cURL,g_iSeqID,cSessionBuf
        );
    }else{
        SetResponse("TEARDOWN",cURL);
    	   sprintf(sRtspSendParam.sendBuf,
            "TEARDOWN %s RTSP/1.0"CRLF
            "CSeq: %d"CRLF
            "User-Agent: Lavf57.28.100"CRLF
            "Session: %s"CRLF
            "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\""CRLF
            CRLF,cURL,g_iSeqID,cSessionBuf,
            ucUserName,ucRealm,ucNonce,cURL,ucResponse
        ); 
    }
    
    printf("Sent TEARDOWN request:\n%s strlen:%d\n", sRtspSendParam.sendBuf,strlen(sRtspSendParam.sendBuf));
	sRtspSendParam.sendVal = send(g_RtspSessionTcpSock, sRtspSendParam.sendBuf, strlen(sRtspSendParam.sendBuf), MSG_NOSIGNAL);
	if (SOCKET_ERROR == sRtspSendParam.sendVal){
        	 printf("send DoTearDown failed!\n") ;
        	 return 0;
	}
	int res = recv(g_RtspSessionTcpSock,sRtspSendParam.recvBuf,sizeof(sRtspSendParam.recvBuf),0);
	if(res == -1){
       printf("Do TEARDOWN error \n");
	   return -1;
	}
    printf("TEARDOWN.recvBuf\n%s\n",sRtspSendParam.recvBuf);

    if(strstr(sRtspSendParam.recvBuf,"RTSP/1.0 200 OK")!=NULL){
       printf("连接关闭成功\n");
       return 1;
    }

   return 0;
}


