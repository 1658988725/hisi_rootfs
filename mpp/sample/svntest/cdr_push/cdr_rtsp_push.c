/************************************************************************	
** Filename: 	cdr_rtsp_push.c
** Description:  
** Author: 	xjl
** Create Date: 2016-12-7
** Version: 	v1.0
   Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>    
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "sample_comm.h"
#include "cdr_comm.h"
#include "h264.h"
#include "cdr_bubiao_analyze.h"
#include "rtsp_client.h"
#include "cdr_rtsp_push.h"

#define RTP_OVER_UDP     0x01
#define SOCKET_BLOCK     0x00
#define SOCKET_NONBLOCK  0x01

char rtp_over_udp = 0x00;
char easy_push_flag = 0;

int g_RtspSessionTcpSock;
int g_RtpSessionUdpSock;
int g_RtpSessionTcpSock;

struct sockaddr_in CameraTcpSockInAddr;

struct sockaddr_in adr_srvr;       /* AF_INET */
static int len_inet;			    /* length  */
static unsigned short seq_num =0;  //�����ʼֵ��Ϊ����
static unsigned int ts_current = 0;//ʱ���

char cRtspSeverIP[16] = {0};
char cRtspSeverPort[6] = {0};

extern unsigned char g_ucRealtimePushAVFlag;
extern int g_iUDPSeverPortOfRtpSession;
extern unsigned char ucUserName[20];
extern unsigned char ucPasswd[20];

typedef struct{  
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested) 0x00 00 00 01 ,0x00 00 01 
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)  
	unsigned max_size;            //! Nal Unit Buffer size 
	int forbidden_bit;            //! should be always FALSE  
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx  
	int nal_unit_type;            //! NALU_TYPE_xxxx      
	char *buf;                    //! contains the first byte followed by the EBSP 
	unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;

RTP_FIXED_HEADER  *rtp_hdr;
NALU_HEADER		*nalu_hdr;
FU_INDICATOR	    *fu_ind;
FU_HEADER		*fu_hdr;

//ΪNALU_t�ṹ������ڴ�ռ�
NALU_t *AllocNALU(int buffersize)
{
  NALU_t *sNaluBuff;
  if ((sNaluBuff = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL) {
	  printf("AllocNALU fails!\n");
	  return NULL;
  }
  
  sNaluBuff->max_size = buffersize;
  if ((sNaluBuff->buf = (char*)calloc (buffersize, sizeof (char))) == NULL) {
    printf ("AllocNALU sNaluBuff->buf fails\n");
    free(sNaluBuff);
	sNaluBuff=NULL; 
	return NULL;
  }
  return sNaluBuff;
}

void FreeNALU(NALU_t *n)
{
  if(n != NULL){
    if(n->buf != NULL){	  
      free(n->buf);
      n->buf=NULL;
    }
    free(n);
	n = NULL;
  }
}

void SetSocketBlockSW(int sockfd,unsigned char mode)
{
	unsigned long ul;
	if(mode == 0x01) ul = 1; //����Ϊ������ģʽ
	if(mode == 0x00) ul = 0; //����Ϊ����ģʽ
    ioctl(sockfd, FIONBIO, &ul); 
}

//init client of  rtsp session 
int TcpSockInit(char* pServerIP,int iServerPort)
{	
    //unlink("g_RtspSessionTcpSock"); 
    g_RtspSessionTcpSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(-1 == g_RtspSessionTcpSock){
	  printf("socket failed! %d \n",errno);
	  return 0;
	} 	
	CameraTcpSockInAddr.sin_family = AF_INET;
    	CameraTcpSockInAddr.sin_addr.s_addr = inet_addr(pServerIP);
    	CameraTcpSockInAddr.sin_port = htons(iServerPort);
	SetSocketBlockSW(g_RtspSessionTcpSock,0x00);//block
	HI_BOOL ret = HI_FALSE;	
	if(connect(g_RtspSessionTcpSock, (struct sockaddr *)&CameraTcpSockInAddr,sizeof(CameraTcpSockInAddr)) == 0){
         printf("connect ok\n");
		ret = HI_TRUE;
	}else{
	    printf("connect fails\n");
		ret = HI_FALSE;
	}
	if(!ret){   
		close(g_RtspSessionTcpSock);
		fprintf(stderr , "Connect failed! errno %d \n",errno);	
		g_RtspSessionTcpSock = -1;
		return 0; 
   }
    
   int rcvbuf_len = 1024*2;  //ʵ�ʻ�������С
   int rcvlen = sizeof(rcvbuf_len);
   if( setsockopt( g_RtspSessionTcpSock, SOL_SOCKET, SO_RCVBUF, (void *)&rcvbuf_len, rcvlen ) < 0 ){
	  perror("getsockopt: SO_RCVBUF");
	  return -1;
   }

    struct timeval tm;
    tm.tv_sec = 5;
    tm.tv_usec = 0;
    if (setsockopt( g_RtspSessionTcpSock, SOL_SOCKET, SO_RCVTIMEO, (void *)&tm, sizeof(tm)) < 0) {   
        perror("getsockopt: SO_RCVTIMEO");
        return -1;
    }
    
    #if(0)
    int enable = 0;
    if(setsockopt(g_RtspSessionTcpSock, SOL_SOCKET, TCP_NODELAY, (void*)&enable, sizeof(enable)) <0){   
        perror("getsockopt: TCP_NODELAY");
        return -1;
    }
    #else
    int on = 1;
    if(setsockopt(g_RtspSessionTcpSock, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on))<0){
        perror("getsockopt: SO_KEEPALIVE");
        return -1;
    }  
    if(setsockopt(g_RtspSessionTcpSock, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on))<0){
        perror("getsockopt: TCP_NODELAY");
        return -1;
    } 
    #endif
   return 1;
}

/*
get ip urs pwd  from uri
char *cURL="rtsp://120.76.30.19:1935/live/Si07bNU5S_ylSypQqZ0Fyg";
rtsp://eyesrc:158Hostm7d@120.76.30.19:1935/live/Si07bNU5S_ylSypQqZ0Fyg/va
*/
int GetPushSeverIP(char *pSrc,char*pDst)
{
      char cBuffTemp[400] = {0};
      char cBuffUserPwd[100] = {0};
      int i = 0;
      int j = 0;
      
      if(pSrc==NULL || pDst==NULL) return -1;

     char *pBuffTemp2 = strstr(pSrc,"@");   
     if(pBuffTemp2 == NULL){
        //printf("no have usr pwd\n");
        char *pBuffTemp = strstr(pSrc,"//");  
        //printf("pBuffTemp %s\n",pBuffTemp);
        pBuffTemp++;
        pBuffTemp++;         
        for(i=0;i<17;i++){
          if(*pBuffTemp ==':')break;
          cBuffTemp[i] = *pBuffTemp++;
        }
        cBuffTemp[i] = '\0';        
      }else{
        //printf("have usr pwd\n");
        //1,get usr pwd
        sscanf(pSrc, "%*[^/]/%[^@]", cBuffUserPwd);
        //printf("cBuffUserPwd:%s\n",cBuffUserPwd);// /eyesrc:158Hostm7d
        i=0;
        j=1;
        for(i=0;i<20;i++){
         if(cBuffUserPwd[j] == ':')break;
         ucUserName[i] = cBuffUserPwd[j];
         j++;
        }
        ucUserName[i] = '\0';
        //printf("usr :%s\n",ucUserName);

        char *cBuffPwd = strstr(cBuffUserPwd,":");
        cBuffPwd++;
        //printf("pwd:%s\n",cBuffPwd);
        memcpy(ucPasswd,cBuffPwd,strlen(cBuffPwd));

        //2,get ip
        pBuffTemp2++;
        i = 0; 
        for(i=0;i<17;i++){
        if(*pBuffTemp2 ==':')break;
        cBuffTemp[i] = *pBuffTemp2++;
        }
        cBuffTemp[i] = '\0';   
     }
         
     //printf("IP:%s\n",cBuffTemp);
     memcpy(pDst,cBuffTemp,strlen(cBuffTemp));

     return 0;
}

//get sever port of rtp session 
int GetPushServerPort(char *pSrc)
{
    int iSeverPort = 0;
    char cBuffTemp[6] = {0};
    char *pBuffTemp = strrchr(pSrc,':');//get the last :
    if(pBuffTemp==NULL){
       printf("get port fails\n");
       return -1;
    }
    //printf("pBuffTemp :%s\n",pBuffTemp);
    pBuffTemp++;
    int i = 0; 
    for(i=0;i<7;i++){
     if(*pBuffTemp =='/')break;
     cBuffTemp[i] = *pBuffTemp++;
    }
    cBuffTemp[i] = '\0';
    iSeverPort = atoi(cBuffTemp);
    //printf("SeverPort:%d\n",iSeverPort);   
    return iSeverPort;
}

/*
get url cut out usr and pwd
�������û���������ʱ��rtsp session  urlҪȥ���û���������
*/
int GetNewUrl(char *pSrc,char *pDst)
{
    char cUrlBuffTemp[200] = {0};    
    char *pDst2 = strstr(pSrc,"@");
    if(pDst2!=NULL){
       pDst2++;
       sprintf(cUrlBuffTemp,"rtsp://%s",pDst2);       
    }else{
       sprintf(cUrlBuffTemp,"%s",pSrc); //no usr pwd      
    }
    memcpy(pDst,cUrlBuffTemp,strlen(cUrlBuffTemp));//no usr pwd
    //printf("g_cUrlBuff:%s\n",pDst);
    return 0;
}

void socketkeepalive(int sockfd)
{
    int keepAlive=1;      //����keepalive����
    int keepIdle = 24;    //���������5����û���κ����������������̽��
    int keepInterval = 2; //̽��ʱ������ʱ����Ϊ2��
    int keepCount = 10;   //̽�Ⳣ�ԵĴ����������1��̽������յ���Ӧ�ˣ����2�εĲ��ٷ���
    int rest = 0;
    
    if(setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,(void *)&keepAlive,sizeof(keepAlive))!=0)//���޴�������setsockopt()����ֵΪ0
    {
        rest = setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,(void *)&keepAlive,sizeof(keepAlive));
		
        printf("It is SO_KEEPALIVE WRONG! %d %d\n",rest,errno);
        //exit(1);
    }
    if(setsockopt(sockfd,SOL_TCP,TCP_KEEPIDLE,(void *)&keepIdle,sizeof(keepIdle))!=0)
    {    
        rest = setsockopt(sockfd,SOL_TCP,TCP_KEEPIDLE,(void *)&keepIdle,sizeof(keepIdle));
        printf("It is TCP_KEEPIDLE WRONG! %d %d\n",rest,errno);
        //exit(1);
    }
    if(setsockopt(sockfd,SOL_TCP,TCP_KEEPINTVL,(void *)&keepInterval,sizeof(keepInterval))!=0)
    {
        rest = setsockopt(sockfd,SOL_TCP,TCP_KEEPINTVL,(void *)&keepInterval,sizeof(keepInterval));
        printf("It is TCP_KEEPINTVL WRONG! %d %d\n",rest,errno);
        //exit(1);
    }
    if(setsockopt(sockfd,SOL_TCP,TCP_KEEPCNT,(void *)&keepCount,sizeof(keepCount))!=0)
    {    
        rest = setsockopt(sockfd,SOL_TCP,TCP_KEEPCNT,(void *)&keepCount,sizeof(keepCount));
        printf("It is TCP_KEEPCNT WRONG! %d %d\n",rest,errno);
        //exit(1);
    }
}

int InitUdpSockOfRtpSession()
{    
    memset(&adr_srvr,0,sizeof adr_srvr);
    adr_srvr.sin_family = AF_INET;
    adr_srvr.sin_port = htons(g_iUDPSeverPortOfRtpSession);             //the port get from rtsp session
    adr_srvr.sin_addr.s_addr = inet_addr(cRtspSeverIP);
    len_inet = sizeof(adr_srvr);
    g_RtpSessionUdpSock = socket(AF_INET,SOCK_DGRAM,0);//udp socket
    
    printf("��ʼ����\n");
    return 0;
}

/*
tcp ������ʧ�ܣ��򷵻�0
*/
int InitTcpSockOfRtpSession(char *rtsp_server_ip,int iSeverPort)
{
    printf("rtsp sever ip:%s sever port :%d\n",rtsp_server_ip,iSeverPort);
    //��˼�����rtsp server ��˼����Ϊһ���ͻ���
    g_RtpSessionTcpSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(-1 == g_RtpSessionTcpSock){
	  printf("socket failed! %d \n",errno);
	  return 0;
	} 	
    
	adr_srvr.sin_family = AF_INET;
	adr_srvr.sin_addr.s_addr = inet_addr(rtsp_server_ip);
	adr_srvr.sin_port = htons(iSeverPort);

	SetSocketBlockSW(g_RtpSessionTcpSock,0x00);
    
	HI_BOOL ret = HI_FALSE;	
	if(connect(g_RtpSessionTcpSock,(struct sockaddr *)&adr_srvr,sizeof(adr_srvr)) == 0){
		ret = HI_TRUE;
         printf("rtp over tcp session connect ok!\n");
	}else{
		ret = HI_FALSE;
	}

	if(!ret)	{   
		close(g_RtpSessionTcpSock);
		fprintf(stderr , "Connect failed! errno %d \n",errno);	
		g_RtpSessionTcpSock = -1;
		return 0;
	}
   
   int rcvbuf_len = 1024*1024*2;  //ʵ�ʻ�������С 2M
   int rcvlen = sizeof(rcvbuf_len);
   if( setsockopt( g_RtpSessionTcpSock, SOL_SOCKET, /*SO_RCVBUF*/SO_SNDBUF, (void *)&rcvbuf_len, rcvlen ) < 0 ){
	  perror("getsockopt: ");
	  return -1;
   }   
   
   int nZero=0;
   setsockopt(g_RtpSessionTcpSock,SOL_SOCKET,SO_SNDBUF,(char *)&nZero,sizeof(nZero));
   
   //socketkeepalive(g_RtpSessionTcpSock);   
   return 1;
}



char g_cUrlBuff[200] = {0};//this url is cut out usr and pwd
#define DELAY_TIME  2
extern int StartPushActiveTime();
extern int g_iSeqID;

void InitRtspPush(char *url)
{
    int res = 0;
    int iSeverPort = 0;
      
    GetPushSeverIP(url,cRtspSeverIP);
    iSeverPort = GetPushServerPort(url);
    GetNewUrl(url,g_cUrlBuff); 

    if(g_ucRealtimePushAVFlag == 0x01) return;
        
    TcpSockInit(cRtspSeverIP,iSeverPort);    

    res = SendOpationRequest(g_cUrlBuff);      
    if(res!=0x01){
        if(g_RtspSessionTcpSock!=-1){
           close(g_RtspSessionTcpSock);
           g_RtspSessionTcpSock = -1;
        }
        g_KakaAckBag.ucResult = 0x01;
        return;        
    }
    //printf("222%d\n",g_iSeqID);
    res = DoAnnounce1(g_cUrlBuff);	
    if(res!=0x01){
        if(g_RtspSessionTcpSock!=-1){
           close(g_RtspSessionTcpSock);
           g_RtspSessionTcpSock = -1;
        }
        g_KakaAckBag.ucResult = 0x01;
        return;        
    }
    //printf("333%d\n",g_iSeqID);
    res = DoAnnounce2(g_cUrlBuff);	
    if(res!=0x01){
        if(g_RtspSessionTcpSock!=-1){
           close(g_RtspSessionTcpSock);
           g_RtspSessionTcpSock = -1;
        }
        g_KakaAckBag.ucResult = 0x01;
        return;
    }
    //printf("444%d\n",g_iSeqID);
    //sleep(DELAY_TIME);
    res = DoSetUp(g_cUrlBuff);	    
    if(res!=0x01){
        if(g_RtspSessionTcpSock!=-1){
           close(g_RtspSessionTcpSock);
           g_RtspSessionTcpSock = -1;
        }
        g_KakaAckBag.ucResult = 0x01;
        return;
    }
    //sleep(DELAY_TIME);
    //printf("555%d\n",g_iSeqID);
    res = DoRecord(g_cUrlBuff);         
    if(res!=0x01){
        if(g_RtspSessionTcpSock!=-1){
           close(g_RtspSessionTcpSock);
           g_RtspSessionTcpSock = -1;
        }
        g_KakaAckBag.ucResult = 0x01;
        return;
    }

    //ok ��������²���
    g_ucRealtimePushAVFlag = 1;   
    if(rtp_over_udp == 0x01){
     InitUdpSockOfRtpSession();   
    }
    StartPushActiveTime();    
}

#if(1)
static int sk_errno (void)
{
    return (errno);

}

static const char *sk_strerror (int err)
{
    return strerror(err);
}
#endif

typedef struct {
	unsigned char magic;
	unsigned char channel;
	unsigned short leng;
}rtp_frame_tcp;

static int rtp_tx_data (const char *data, int size)
{
    int ret = -1;
	if (rtp_over_udp == 0x00){
		
		static rtp_frame_tcp rtp_frame = {'$',0x00,0x00};
		rtp_frame.leng = htons(size);

		ret = send(g_RtspSessionTcpSock, (const char*)&rtp_frame, 4, MSG_NOSIGNAL);
		if (ret == SOCKET_ERROR) {
		    if (sk_errno() != SK_EAGAIN && sk_errno() != SK_EINTR) {
			    printf("rtp over tcp send interlaced frame to %s failed: %s\n", 
                        cRtspSeverIP, sk_strerror(sk_errno()));
			    return -1;
            }
            return 0;
		}

		ret = send(g_RtspSessionTcpSock, (const char*)data, size, MSG_NOSIGNAL);
		if (ret == SOCKET_ERROR) {
		    if (sk_errno() != SK_EAGAIN && sk_errno() != SK_EINTR) {
			    printf("rtp over tcp send %d bytes to %s failed: %s\n", size, cRtspSeverIP, sk_strerror(sk_errno()));
			    return -1;
            }
            return 0;
		}
	} else {
         ret = sendto(g_RtpSessionUdpSock,data,size,0,(struct sockaddr *)&adr_srvr,len_inet);
		if (ret == SOCKET_ERROR) {
		    if (sk_errno() != SK_EAGAIN && sk_errno() != SK_EINTR) {
			    printf("rtp over udp send %d bytes to %s failed: %s\n", size, cRtspSeverIP, sk_strerror(sk_errno()));
			    return -1;
            }
            return 0;
		}
	}
	return size;
}



int Push_RtpSendFrame(unsigned char *pFrameBuffer,unsigned int uiFrameLens,unsigned long long pts)
{
    char* nalu_payload = NULL;            //����
    char sendbuf[1500] = {0};             //udp���͵�buffer
    int	bytes = 0;                       //���͵����ݳ��ȼ�¼��    	
    unsigned int timestamp_increse = 3333;//3000;//(unsigned int)(90000.0/30);  // ʱ������� (90000.0 /framerate)
    NALU_t * sNaluBuff = AllocNALU(300000);
           
	//step 2 get nalu
	sNaluBuff->startcodeprefix_len = 4;
	if(uiFrameLens > 4)
	{
	  sNaluBuff->len = uiFrameLens - (sNaluBuff->startcodeprefix_len);
	  memcpy (sNaluBuff->buf, pFrameBuffer+4, sNaluBuff->len); 
	  sNaluBuff->forbidden_bit = sNaluBuff->buf[0] & 0x80;
	  sNaluBuff->nal_reference_idc = sNaluBuff->buf[0] & 0x60; // 2 bit nal_ref_idc. ȡ 00 ~ 11, �ƺ�ָʾ��� NALU ����Ҫ��, �� 00 �� NALU ���������Զ���������Ӱ��ͼ��Ļط�. ����һ������²�̫�����������.
	  sNaluBuff->nal_unit_type = (sNaluBuff->buf[0]) & 0x1f;// 5 bit nal_unit_type. ��� NALU ��Ԫ������
	}else{
       printf("this data's lens is not a frame\n");
       FreeNALU(sNaluBuff);
      return -1;
	}

    //step 3 start send
	memset(sendbuf,0,1500);
	rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0];	
	rtp_hdr->payload     = H264;         //�������ͺţ�
	rtp_hdr->version     = 2;            //�汾�ţ��˰汾�̶�Ϊ2
	rtp_hdr->marker      = 0;            //��־λ���ɾ���Э��涨��ֵ��
	rtp_hdr->ssrc        = htonl(10);    //���ָ��Ϊ10�������ڱ�RTP�Ự��ȫ��Ψһ

	if((sNaluBuff->nal_unit_type == 1)||(sNaluBuff->nal_unit_type == 7))
	{
		ts_current = ts_current+timestamp_increse;
		//usleep((1000/framerate)*666+3.4);//Ч���Ϻã���βʱ�е㿨
	}  	
	if(sNaluBuff->len<=MAX_RTP_PKT_LENGTH) 
	{
		rtp_hdr->marker=1;
		rtp_hdr->seq_no = htons(seq_num ++); //���кţ�ÿ����һ��RTP����1

		nalu_hdr =(NALU_HEADER*)&sendbuf[12]; 
		nalu_hdr->F=sNaluBuff->forbidden_bit;
		nalu_hdr->NRI=sNaluBuff->nal_reference_idc>>5;
		nalu_hdr->TYPE=sNaluBuff->nal_unit_type;

		nalu_payload=&sendbuf[13];
		memcpy(nalu_payload,sNaluBuff->buf+1,sNaluBuff->len-1);

		rtp_hdr->timestamp=htonl(ts_current);
		bytes=sNaluBuff->len + 12 ;

         rtp_tx_data(sendbuf,bytes);

	}else{
         int k = 0;
         int l = 0;              //�õ���nalu��Ҫ�ö��ٳ���Ϊ1400�ֽڵ�RTP��������
		k = sNaluBuff->len/MAX_RTP_PKT_LENGTH;//��Ҫk��1400�ֽڵ�RTP��
		l = sNaluBuff->len%MAX_RTP_PKT_LENGTH;//���һ��RTP������Ҫװ�ص��ֽ���

		if(l > 0) k++;          // �˴���4800�� k = 2 ,l=0, 4802 k= 2,l =2,-->k =3,l =2;
		int t = 0;              //����ָʾ��ǰ���͵��ǵڼ�����ƬRTP��
		rtp_hdr->timestamp = htonl(ts_current);

		while(t < k)//k��ȡֵ��2��ʼ��
		{
			rtp_hdr->seq_no = htons(seq_num ++); //���кţ�ÿ����һ��RTP����1
			if(!t)                           
			{
				rtp_hdr->marker=0;                       //����rtp M λ��
				fu_ind =(FU_INDICATOR*)&sendbuf[12];
				fu_ind->F=sNaluBuff->forbidden_bit;
				fu_ind->NRI=sNaluBuff->nal_reference_idc>>5;
				fu_ind->TYPE=28;
				
				fu_hdr =(FU_HEADER*)&sendbuf[13];//����FU HEADER,�������HEADER����sendbuf[13]
				fu_hdr->E=0;
				fu_hdr->R=0;
				fu_hdr->S=1;
				fu_hdr->TYPE=sNaluBuff->nal_unit_type;

				nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]����nalu_payload
				memcpy(nalu_payload,sNaluBuff->buf+1,MAX_RTP_PKT_LENGTH);//ȥ��NALUͷ
				bytes=MAX_RTP_PKT_LENGTH+14;
			}else if(t < (k - 1)){
				rtp_hdr->marker=0;
				
				fu_ind =(FU_INDICATOR*)&sendbuf[12]; 
				fu_ind->F=sNaluBuff->forbidden_bit;
				fu_ind->NRI=sNaluBuff->nal_reference_idc>>5;
				fu_ind->TYPE=28;
				
				fu_hdr =(FU_HEADER*)&sendbuf[13];
				fu_hdr->R=0;
				fu_hdr->S=0;
				fu_hdr->E=0;
				fu_hdr->TYPE=sNaluBuff->nal_unit_type;

				nalu_payload=&sendbuf[14];
				memcpy(nalu_payload,sNaluBuff->buf+t*MAX_RTP_PKT_LENGTH+1,MAX_RTP_PKT_LENGTH);
				bytes=MAX_RTP_PKT_LENGTH+14;	
			}else{
				rtp_hdr->marker=1;

				fu_ind =(FU_INDICATOR*)&sendbuf[12];
				fu_ind->F=sNaluBuff->forbidden_bit;
				fu_ind->NRI=sNaluBuff->nal_reference_idc>>5;
				fu_ind->TYPE=28;
				
				fu_hdr =(FU_HEADER*)&sendbuf[13];
				fu_hdr->R=0;
				fu_hdr->S=0;
				fu_hdr->TYPE=sNaluBuff->nal_unit_type;
				fu_hdr->E=1;
				nalu_payload=&sendbuf[14];

				if (l==0)l= MAX_RTP_PKT_LENGTH;
				memcpy(nalu_payload,sNaluBuff->buf+t*MAX_RTP_PKT_LENGTH+1,l-1);				
				bytes=l-1+14;				
			}
             rtp_tx_data(sendbuf,bytes);
			t++;
		 }		
	  }
     FreeNALU(sNaluBuff);	
    return 0;
}


