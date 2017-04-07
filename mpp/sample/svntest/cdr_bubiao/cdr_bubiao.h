
#ifndef BUDIAO_H
#define BUDIAO_H

#include "cdr_count_file.h"
#include "cdr_bubiao_analyze.h"


extern unsigned short usResponseSerialNm;
void InitAckBag();

/*
存储媒体概要信息查询应答
所应答流水号	WORD	
最早时间	BCD[6]	所查询类型的文件的最早时间
最后时间	BCD[6]	所查询类型的文件的最后时间
文件数量	BYTE	所查询类型的文件的数量
*/
typedef struct{
    unsigned char  ucMediaType;
    unsigned char  ucChnID;
    unsigned char  ucStartTime[15]; //BCD[6] 14 15  若用bcd 6会存在bug ,1970 年占用2个字节
    unsigned char  ucEndTime[15]; 
    unsigned short usFileCnt;
}s_MediaProfileInformBody;

s_MediaProfileInformBody g_sMediaProfileBody;
int MediaProfileInformQueryProcess();


/*
表85 存储多媒体数据检索消息体数据格式
起始字节	字段	数据类型	描述及要求
0	多媒体类型	BYTE	0：图像；1：音频；2：视频；
1	通道ID	BYTE	0 表示检索该媒体类型的所有通道；
2	事件项编码	BYTE	0：平台下发指令；1：定时动作；2：抢劫报警触发；3：碰撞侧翻报警触发；其他保留
3	起始时间	BCD[6]	YY-MM-DD-hh-mm-ss
9	结束时间	BCD[6]	YY-MM-DD-hh-mm-ss
*/
typedef struct{
    unsigned char  ucMediaType;  
    unsigned char  ucChnID;      
    unsigned char  uEventItemCode;
    unsigned char  ucStartTime[15]; //BCD[6] 14
    unsigned char  ucEndTime[15]; 
    unsigned short usMediaBagCnt;
    unsigned short usMediaDataTatoil;  //多媒体数据总项数  2017 03 02新增
    unsigned short usPreBagMediaDataTatoil;  //每个返回包多媒体数据总项数  2017 03 02新增

    unsigned char  ucFileName[100];
    unsigned short usVideoLen;  
}s_DataSearchMsgBody;

s_DataSearchMsgBody g_sDataSearchMsgBag;
unsigned short  g_usMediaDataTatoil;  //多媒体数据总项数


int MediaDataInformQueryProcess();

/*
动作	BYTE	0：停止下载；1：开始下载 
文件名长度	BYTE	
文件名	BYTE[…]	文件名ASCII字节数组，包含扩展名
文件类型	BYTE	1：大图；2：大图缩略图；3：独立视频；4：独立视频预览图（一张）
上传URL长度	BYTE	
上传URL	BYTE[…]	
*/
typedef struct{
    unsigned char  ucUploadActive; 
    unsigned char  ucChnID;             
    unsigned char  ucFileNameLen;      
    unsigned char  ucFileName[45];
    unsigned char  ucFileType;
    unsigned char  ucUploadURLLen;
    unsigned char  ucUploadURL[255]; 
}s_MediaUploadAttr;

//s_MediaUploadAttr g_sMediaUploadAttrBody;
int MediaUploadQueryProcess();

/*
动作	BYTE	0：停止上传；1：开始上传
开始时间点	BCD[6]	
时长	WORD	单位：秒
文件类型	BYTE	1：循环视频；2：循环视频预览图
上传URL长度	BYTE	
上传URL	BYTE[…]	
*/
typedef struct{
    unsigned char  ucUploadActive;  
    unsigned char  ucChnID;                 
    unsigned char  ucStartTime[15]; //BCD[6] 17-01-23 15-17-59 (6个字节)此字段接收到的数据要经过转换变成20170221 121350 (15个字节)型式
    unsigned short  usAVTimeLen;
    unsigned char  ucEndTime[15]; //新增
    unsigned char  ucFileType;
    unsigned char  ucUploadURLLen;
    unsigned char  ucUploadURL[255]; 

    unsigned char  ucResult;
    char sAreaMediaUploadFileName[100];//存放区域媒体上传对应的文件名
}s_AreaMediaUploadAttr;

s_AreaMediaUploadAttr g_sAreaMediaUploadAttrBody;
int AreaMediaUploadQueryProcess();


/*
起始字节	字段	数据类型	描述及要求
0	逻辑通道号	BYTE	按照JT/T 1076-2016中的表2.0表示所有通道
1	开始时间	BCD[6]	YY-MM-DD-HH-MM-SS，全0表示无起始时间条件
7	结束时间	BCD[6]	YY-MM-DD-HH-MM-SS，全0表示无终止时间条件
13	报警标志	64BITS	报警标志位定义见表24
全0表示不指定是否有报警
21	音视频资源类型	BYTE	0：音视频，1：音频，2：视频，3：视频或者音视频
22	码流类型	BYTE	0：所有码流，1：主码流，2：子码流
23	存储器类型	BYTE	0：所有存储器，1：主存储器，2：灾备存储器
*/
typedef struct{
    unsigned char  ucLogicChnID;  
    unsigned char  ucStartTime[15]; 
    unsigned char  ucEndTime[15]; 
    unsigned char  ucWarningMark[8];
    unsigned char  ucSourceType;
    unsigned char  ucStreamType;
    unsigned char  ucStoreType;    
}s_SearchSourceListBody;

s_SearchSourceListBody g_sSearchSourceListBody;


/*
服务器IP地址长度 	BYTE 	长度n 
服务器IP地址 	STRING 	实时音视频服务器IP地址 
服务器音视频通道监听端口号（TCP） 	WORD 	实时音视频服务器端口号,不适用TCP传输时置0
服务器音视频通道监听端口号（UDP） 	WORD 	实时音视频服务器端口号,不适用UDP传输时置0
逻辑通道号 	BYTE 	2：车辆正前方 
音视频类型	BYTE 	0：音视频；1：音频；2：视频；3：视频或音视频
码流类型 	BYTE 	0：主码流或子码流；1：主码流；2：子码流；如果此通道只传输音频，此字段置0 
回放方式 	BYTE 	0：正常回放；
1：快进回放； 
2：关键帧快退回放；
3：关键帧播放；  
4：单帧上传 
快进或快退倍数 	BYTE 	回放方式为1和2时，此字段内容有效，否则置0。 
0：无效；
1：1倍；
2：2倍；
3：4倍；
4：8倍；
5：16倍
开始时间 	BCD[6] 	YY--DD-HH-NN-SS，回放方式为4时，该字段表示单帧上传时间 
结束时间 	BCD[6] 	YY-MM-DD-HH-NN-SS，为0表示一直回放，回放方式为4时，该字段无效
*/
typedef struct{
    unsigned char  ucServerIPAddrLen; //服务器IP地址长度
    char  usServerIPAddr[200];        //192.168.166.120 -15 \0 url  实时音视频服务器IP地址
    unsigned short usServerTCPListenPort;
    unsigned short usServerUDPListenPort;
    unsigned char  ucAVChanlNm;          //2：车辆正前方
    unsigned char  ucMediumType;         //0x00 :vodio 0x01:audio 0x02:picture
    unsigned char  ucStreamType;    
    unsigned char  ucPlayBackWay;        //回放方式
    unsigned char  ucSpeedNum;           //快进快退倍数
    
    unsigned char  ucStartTime[15];
    unsigned char  ucEndTime[15];     
}s_VideoPlayBackBody;    



/*
音视频通道号	BYTE	按照JT/T 1076-2106中的表2
回放控制	BYTE	0：开始回放
1：暂停回放
2：结束回放
3：快进回放
4：关键帧快退回放
5：拖动回放
6；关键帧播放
快进或快退倍数	BYTE	回放控制为3和4时，此字段内容有效，否则置0。
0：无效；
1:1倍；
2:2倍；
3:4倍；
4:8倍；
5:16倍；
拖动回放位置	BCD[6]	YY-MM-DD-HH-MM-SS，回放控制为5时，此字段有效
*/
typedef struct{
    unsigned char  ucAVChnID; 
    unsigned char  ucPlayBackControl;            //0：开始回放1：暂停回放2...
    unsigned char  ucSpeedNum;                   //快进快退倍数    
    unsigned char  ucPlayBackStartTime[15];      //拖动回放位置
}s_VideoPlayBackControlBody; 

/*
部标查询的结果存储的结构体
*/
typedef struct{
  unsigned char ucMsgReturnFlag;

  unsigned short usCMDMsgID;
  unsigned short usGeneralMsgID;
  
  s_MediaProfileInformBody sMediaProfileBody;
  s_MediaUploadAttr sMediaUploadAttrBody;
  s_AreaMediaUploadAttr sAreaMediaUploadAttrBody;
  
  sResponseBag sResponsePack;
  sKaKaAckBag sKakaAckPack;
  s_EyeExtendedProtocol sEyeExtendedProtocolBag;
  s_KaKaRalation sKaKaRalationBag;

  s_DataSearchMsgBody sDataSearchMsgBag;
  sSearchTotailMediaItemBuf *pSearchTotailMediaItemBuf;  

  s_SearchSourceListBody sSearchSourceListBody;
  s_VideoPlayBackBody sVideoPlayBackBody;
  
}s_BuBiaoTatoilResult;

typedef struct {
	sBBMsgHeaderPack pBBUploadInfoHeader;
	s_BuBiaoTatoilResult sTatoiResultPackageBuf;
} sBBUploadInfo;


s_BuBiaoTatoilResult g_TatoiResultPackage;

int SearchMediaProcess(s_BuBiaoTatoilResult *sTatoiResultPackage,sBBMsgHeaderPack sBBMsgHeadPack);
int GetMsgBodyPackage(sBBMsgArr *sMsgBodyArr,sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult sTatoiResultPackage);
int GetBBAckMsgHeadFromBBRevMsgHead(sBBMsgHeaderPack sBBMsgHeadPack,
    sBBMsgHeaderPack *sBBAckMsgHeadPack,unsigned char ucSubBagFlag,unsigned short usMsgBodyLen,
    unsigned short usMsgBagCnt,unsigned short usMsgBagIndex);
unsigned char GetSubBagFlag(unsigned short usMsgBodyLen);
int GetAckArrFromBBAckMsgHead(sBBMsgArr *sMsgHeadArr,
    sBBMsgHeaderPack sBBAckMsgHeadPack);

void BuBiaoMsgBusyProcess(sBBMsgArr *sBBMsgArrPack);;
int GetBBRevMsgHeadFromBBMsg(sBBMsgArr sBBERMsgBag,sBBMsgHeaderPack *sBBMsgHeadPack);
int  SendDataTo406(unsigned char *ucData,int iLen);

int SetUrlBuff(char *pcDst,int iLen);

int GetMediaProfileInform(unsigned char ucType,s_BuBiaoTatoilResult *sTatoiResultPackage);

extern int SearchMediaProcessCore(const char *pDir,s_BuBiaoTatoilResult *sTatoiResultPackage,sBBMsgHeaderPack sBBMsgHeadPack);


void AckToPlatform(sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult sTatoiResultPackage);
extern int cdr_ftp_upload(unsigned char* url,unsigned char *FullFileName,sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage);

void cdr_ftp_upload_process(char *file_name);
void cdr_ftp_upload_process_ex(char *file_name,void *pUserData);

int SearchSourceListProcess(sBBMsgArr sBBMsgArrPack,
   sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage);

int VideoPlayBackProcess(sBBMsgArr sBBMsgArrPack,
    sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage);
#endif



