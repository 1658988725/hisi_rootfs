/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#ifndef BUBIAO_H
#define BUBIAO_H

#include "cdr_comm.h"
#include "cdr_count_file.h"



#if(0)
#define CMD_DATA_PASSTHROUGH_UP         0x0900  //数据上行/下行透传 Data passthrough 数据上行用0x0900消息ID封装上传，平台采用“平台通用应答”指令应答
#define CMD_DATA_PASSTHROUGH_REQUEST    0x8900  //数据上行/下行透传 Data passthrough 数据下行用0x8900消息ID封装下发，终端采用“终端通用应答”指令应答，闹钟指令有专用应答指令。
#define CMD_REAL_TIME_AV_REQUEST        0x8B00  //实时音视频请求

#define CMD_MEDIA_INFORM_REQUEST        0x8B80  //概要信息查询指令
#define CMD_DATA_SEARCH_REQUEST         0x8B81  //存储多媒体数据检索
#define CMD_MEDIA_UPLOAD_REQUEST        0x8B82  //平台下发远程媒体上传请求
#define CMD_AREA_MEDIA_UPLOAD_REQUEST   0x8B83  //按时间段请求视频或预览图上传
#else

#define GENERAL_ACK_ID                  0x0001

#define SEARCH_SOURCE_LIST_CMD          0x9205
#define SEARCH_SOURCE_LIST_ACK_CMD      0x1205
#define VIDEO_PLAY_BACK_CMD             0x9201

#define CMD_DATA_PASSTHROUGH_UP         0x0900  //数据上行/下行透传 Data passthrough 数据上行用0x0900消息ID封装上传，平台采用“平台通用应答”指令应答
#define CMD_DATA_PASSTHROUGH_REQUEST    0x8900  //数据上行/下行透传 Data passthrough 数据下行用0x8900消息ID封装下发，终端采用“终端通用应答”指令应答，闹钟指令有专用应答指令。
#define CMD_REAL_TIME_AV_REQUEST        0x9101  //实时音视频请求
#define CMD_REAL_TIME_AV_CONTROL        0x9102  //音视频实时传输控制

#define CMD_MEDIA_INFORM_REQUEST        0xFB80  //概要信息查询指令
#define CMD_DATA_SEARCH_REQUEST         0xFB81  //存储多媒体数据检索
#define CMD_MEDIA_UPLOAD_REQUEST        0xFB82  //平台下发远程媒体上传请求
#define CMD_AREA_MEDIA_UPLOAD_REQUEST   0xFB83  //按时间段请求视频或预览图上传

#endif


#define  ID_KEY                         0x7E
#define  BUBIAO_MAX_LEN                 400//300
#define  BUBIAO_HEAD_MAX_LEN            20
#define  MOBILE_PHONE_LEN               0x06

/*
消息体属性格式结构图如图 2 所示：
15	14	13	12	11	10	9	8	7	6	5	4	3	2	1	0
保留	分包	数据加密方式	消息体长度
*/
typedef struct{
    unsigned short Reserved:2;     
    unsigned short SubPackage:1;     
    unsigned short DataEncryption:3; 
    unsigned short MsgBodyLen:10;
}sBB_MsgBodyAttr;

typedef union{
    struct{
        unsigned short Reserved:2;     
        unsigned short SubPackage:1;     
        unsigned short DataEncryption:3; 
        unsigned short MsgBodyLen:10;
    }sBB_MsgBodyAttr;
    unsigned short usMsgBodyAttr;
}uMsgBodyAttr;

/*
BIT0：0-录像正常；
      1-录像故障；
BIT1：0-TF卡正常
      1-TF卡故障
BIT2：0-麦克正常
      1-麦克故障
BIT3：0-WIFI正常
      1-WIFI故障
BIT4：0-蓝牙正常
      1-蓝牙故障
BIT5：0-遥控按键正常
      1-遥控按键故障
BIT6：1-图像运动检测报警
*/
typedef struct{
    unsigned short RecordStatus:1;     
    unsigned short TFStatus:1;     
    unsigned short MicrophoneStatus:1; 
    unsigned short WifiStatus:1;

    unsigned short BtStatus:1;     
    unsigned short YKKeyStatus:1;     
    unsigned short MotionDetection:1; 
    unsigned short Reserved:10;
}sKaKaStatus;

sKaKaStatus g_sKaKaStatus;


/*
咔咔应答数据

1,
6.26 存储多媒体数据检索应答
消息ID：0x0802。
存储多媒体数据检索应答消息体数据格式见表86。
表86 存储多媒体数据检索应答消息体数据格式
起始字节	字段	数据类型	描述及要求
0	应答流水号	WORD 	对应的多媒体数据检索消息的流水号
2	多媒体数据总项数	WORD 	满足检索条件的多媒体数据总项数
4	检索项		多媒体检索项数据格式见表87

表87 多媒体检索项数据格式
起始字节	字段	数据类型	描述及要求
0	多媒体ID	DWORD	>0
4	多媒体类型	BYTE	0：图像；1：音频；2：视频
5	 通道ID	BYTE	
6	事件项编码	BYTE	0：平台下发指令；1：定时动作；2：抢劫报警触发；3：碰撞侧翻报警触发；其他保留
7	位置信息汇报
(0x0200)消息体	BYTE[28]	表示拍摄或录制的起始时刻的位置基本信息数据

2,
外设应答	0x38	外设(咔咔）编号ID	1	0x00～0x07
		数据长度	1	≧2
		外设状态	2	 
*/
typedef struct{    
    unsigned char ucResult;     
    unsigned char ucCMDWorld;//命令字 固定为0x38
    unsigned char ucKaKaSerialNumID;//外设编号0 - 7
    unsigned char ucDataLen;
    unsigned short usKaKaStatu;
    //sKaKaStatus  mKaKaStatu;
}sKaKaAckBag;

sKaKaAckBag g_KakaAckBag;


/*
起始字节	字段	数据类型	描述及要求
0	消息总包数	WORD	该消息分包后的总包数
2	包序号	WORD	从1开始
*/
#pragma pack(1)
typedef struct{
    unsigned short usMsgTotalPackageNm; 
    unsigned short usMsgPackIndex;
} sBB_MsgPackageItem;              //消息包封装项 4字节
#pragma pack()

sBB_MsgPackageItem g_sBBMsgPackageItems;

//消息头
#pragma pack(1)
typedef struct {
	unsigned short   usMsgID ;            //消息ID u16;
	sBB_MsgBodyAttr  sMsgBodyAttr;        //消息体属性
	unsigned char    ucTerminalPhoneNm[6];//终端手机号
	unsigned short   usMsgSerialNm;       //消息流水号
	sBB_MsgPackageItem    sMsgPackItem;   //消息包封装项,现有包不会存在这个项
	int iMsgHeadLen;                      //消息头长度 额外增加
} sBBMsgHeaderPack;
#pragma pack()

sBBMsgHeaderPack g_sBBMsgHeadPack,g_sBBMsgAckHeadPack;

typedef struct {
	int iMsgLen; 
    unsigned char *ucMsgArr;
}sBBMsgArr;



/*
0	应答流水号	WORD	对应的平台消息的流水号
2	应答ID	WORD	对应的平台消息的ID
4	结果	BYTE	0：成功/确认；1：失败；2：消息有误；3：不支持；
*/
typedef struct{
    unsigned short usResponseSerialNm; 
    unsigned short usResponseID;
    unsigned char  ucResponseResult;
}sResponseBag;

sResponseBag g_sResponsePack;

int AnalysisMsgHard(char *pSrcBuf);


/*
实时音视频请求所带参数的解析
起始字节	字段 	数据类型 	描述及要求 
0 	服务器IP地址长度 	BYTE 	长度n 
1 	服务器IP地址 	STRING 	实时音视频服务器IP地址 
1+n 	服务器音视频通道监听端口号（TCP）	WORD 	实时音视频服务器端口号 
3+n 	服务器音视频通道监听端口号（UDP）	WORD 	实时音视频服务器端口号 
5+n 	音视频通道号 	BYTE 	从1开始 
6+n 	是否携带音频 	BYTE 	0:携带；1:不携带 
7+n 	主/子码流标志 	BYTE 	0:主码流；1:子码流 
8+n 	开启/关闭标志 	BYTE 	0:关闭；1:开启 
*/
typedef struct{
    unsigned char  ucServerIPAddrLen; 
    char  usServerIPAddr[200];  //192.168.166.120 -15 \0 url
    unsigned short usServerTCPListenPort;
    unsigned short usServerUDPListenPort;
    unsigned char  ucAVChanlNm;
    unsigned char  ucMediumType;         //0x00 :vodio 0x01:audio 0x02:picture
    unsigned char  ucStreamType;    
    unsigned char  ucSwitchFlag;

}s_RealTimeAVBody;

s_RealTimeAVBody g_sRealTimeAVBody;


int BuBiaoMsgProcess(sBBMsgArr *sBBMsgArrPack);

/*
音视频实时传输控制数据格式
起始字节	字段 	数据类型 	描述及要求 
0 	逻辑通道号 	BYTE 	2：车辆正前方 
1 	控制指令	BYTE 	0：关闭音视频传输
1：切换码流
2：暂停该通道所有流的发送；
3：恢复暂停前流的发送，与暂停前的流类型一致；
4：关闭双向对讲
127:保持
2	关闭音视频类型	BYTE	0：关闭该通道有关的音视频数据；
1：只关闭该通道有关的音频；
2：关闭该通道有关的视频。
3	切换码流类型	BYTE	音频与切换前保持一致，新申请的码流：
0：主码流；
1：子码流
*/
typedef struct{    
    unsigned char  ucLogicChanlNm;
    unsigned char  ucControlID;       
    unsigned char  ucCloseAVType;         //关闭音视频类型 
    unsigned char  ucSwitchStreamType;    //切换码流类型
}s_RealTimeAVControl;

s_RealTimeAVControl g_sRealTimeAVControl;

/*
extended protocol
识别码	版本号	子协议号	数据区长度	数据区
		功能	命
令		
0x01	0x01	00	0x05		校时
*/
typedef struct{
    unsigned char  ucIDCode;               //识别码
    unsigned char  ucVersionNm;            //版本号
    //unsigned short usSubProtocolNm;
    unsigned char  ucSubProtocolFunction;  //功能
    unsigned char  ucSubProtocolCommond;   //命令
    unsigned short usDataLen;              //数据区长度
    unsigned char  ucEyeExtendData[1024];  //数据 

}s_EyeExtendedProtocol;

s_EyeExtendedProtocol g_sEyeExtendedProtocolBag;

/*
车辆实时信息
1	时间	BCD[6]	BCD码表示年月日时分秒
2	电瓶电压	WORD	单位：mV; 高字节在前
3	发动机转速	WORD	单位：rpm
4	车速	BYTE[1]	单位：km/h
5	节气门开度	BYTE[1]	单位：1%
6	发动机负荷	BYTE[1]	单位：1%
7	冷却液温度	BYTE[1]	单位：℃
8	瞬时油耗	WORD	当车速为零时，表示怠速油耗，单位：L/H；当车速大于零时，表示百公里油耗，单位是0.001L/100KM.<30L/100KM
9	平均油耗	WORD	单位: 0.001L/100KM.<30L/100KM
10	本次行驶里程	DWORD	单位：米
11	总里程	DWORD	单位：米
12	本次耗油量	DWORD	单位：毫升
13	累计耗油量	DWORD	单位：毫升
*/
typedef struct{
    unsigned char  ucTime[6]; 
    unsigned short usBatteryVoltage;

    unsigned short usEngineSpeed; 
    unsigned char  ucCarSpeed;
    unsigned char  ucThrottlePercentage;//节气门开度
    unsigned char  ucEngineLoad;
    unsigned char  ucCoolantTemperature;//冷却液温度

    unsigned short usInstantaneousFuelConsumption;//瞬时油耗
    unsigned short usAverageFuelConsumption;      //平均油耗

    unsigned short usTheMileage;                  //本次行驶里程；
    unsigned short usTotalMileage;
    unsigned short usTheFuelConsumption;          //本次油耗
    unsigned short usTotalFuelConsumption;        //总油耗
    
}s_CarRealtimeInformation;

s_CarRealtimeInformation g_sCarRealtimeInformation;

/*咔咔相关 要求行车记录仪自检及应答
命令字	数据包内容	数据长度	取值
0x38	外设(咔咔）编号ID	1	0x00～0x07
	数据长度	1	≧2
	车辆状态	2	BIT0：0-ACC OFF；
      1-ACC ON；
BIT1：0-车门关
      1-车门开
BIT1：0-未超速
      1-超速
*/
typedef struct{
    unsigned char  ucCmdKey;                  //命令字
    unsigned char  ucPeripheralNmID;          //外设编号
    unsigned char  uDataLen; 
    unsigned short usCarStatus;               //车辆状态
}s_KaKaRalation;

s_KaKaRalation g_sKaKaRalationBag;

typedef struct{
    unsigned char  ucCmdKey;                  //命令字
    unsigned char  ucPeripheralNmID;          //外设编号
    unsigned char  uDataLen; 
    unsigned short usKaKaStatus;              //外设状态

}s_KaKaRalationResponse;

s_KaKaRalationResponse g_sKaKaRalationResponseBag;


/*
部标协议的处理的状态
*/
typedef enum
{
 STATE_USER_IDLE,	  //状态机空闲
 STATE_USER_BUSY,	
 STATE_USER_UNDEFINE,	 
}StateUser;
StateUser g_esBuBaioProcState;


unsigned char GetCapturePictureFlag(void);
unsigned char SetCapturePictureFlag(unsigned char ucValue);

int GetUrlBuff(char *pcDst);
void PrintfString2Hex2(char *pSrc,int len);

void BuBiaoProtoRetuBagHandle();
void BuBiaoProtoRecvBagHandle();

int GetAckPack(sBBMsgArr sMsgAckHeadArr,sBBMsgArr sMsgAckBodyArr,sBBMsgArr *sMsgArr);

#endif



