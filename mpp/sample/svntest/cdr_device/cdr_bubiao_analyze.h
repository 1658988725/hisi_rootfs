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
#define CMD_DATA_PASSTHROUGH_UP         0x0900  //��������/����͸�� Data passthrough ����������0x0900��ϢID��װ�ϴ���ƽ̨���á�ƽ̨ͨ��Ӧ��ָ��Ӧ��
#define CMD_DATA_PASSTHROUGH_REQUEST    0x8900  //��������/����͸�� Data passthrough ����������0x8900��ϢID��װ�·����ն˲��á��ն�ͨ��Ӧ��ָ��Ӧ������ָ����ר��Ӧ��ָ�
#define CMD_REAL_TIME_AV_REQUEST        0x8B00  //ʵʱ����Ƶ����

#define CMD_MEDIA_INFORM_REQUEST        0x8B80  //��Ҫ��Ϣ��ѯָ��
#define CMD_DATA_SEARCH_REQUEST         0x8B81  //�洢��ý�����ݼ���
#define CMD_MEDIA_UPLOAD_REQUEST        0x8B82  //ƽ̨�·�Զ��ý���ϴ�����
#define CMD_AREA_MEDIA_UPLOAD_REQUEST   0x8B83  //��ʱ���������Ƶ��Ԥ��ͼ�ϴ�
#else

#define GENERAL_ACK_ID                  0x0001

#define SEARCH_SOURCE_LIST_CMD          0x9205
#define SEARCH_SOURCE_LIST_ACK_CMD      0x1205
#define VIDEO_PLAY_BACK_CMD             0x9201

#define CMD_DATA_PASSTHROUGH_UP         0x0900  //��������/����͸�� Data passthrough ����������0x0900��ϢID��װ�ϴ���ƽ̨���á�ƽ̨ͨ��Ӧ��ָ��Ӧ��
#define CMD_DATA_PASSTHROUGH_REQUEST    0x8900  //��������/����͸�� Data passthrough ����������0x8900��ϢID��װ�·����ն˲��á��ն�ͨ��Ӧ��ָ��Ӧ������ָ����ר��Ӧ��ָ�
#define CMD_REAL_TIME_AV_REQUEST        0x9101  //ʵʱ����Ƶ����
#define CMD_REAL_TIME_AV_CONTROL        0x9102  //����Ƶʵʱ�������

#define CMD_MEDIA_INFORM_REQUEST        0xFB80  //��Ҫ��Ϣ��ѯָ��
#define CMD_DATA_SEARCH_REQUEST         0xFB81  //�洢��ý�����ݼ���
#define CMD_MEDIA_UPLOAD_REQUEST        0xFB82  //ƽ̨�·�Զ��ý���ϴ�����
#define CMD_AREA_MEDIA_UPLOAD_REQUEST   0xFB83  //��ʱ���������Ƶ��Ԥ��ͼ�ϴ�

#endif


#define  ID_KEY                         0x7E
#define  BUBIAO_MAX_LEN                 400//300
#define  BUBIAO_HEAD_MAX_LEN            20
#define  MOBILE_PHONE_LEN               0x06

/*
��Ϣ�����Ը�ʽ�ṹͼ��ͼ 2 ��ʾ��
15	14	13	12	11	10	9	8	7	6	5	4	3	2	1	0
����	�ְ�	���ݼ��ܷ�ʽ	��Ϣ�峤��
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
BIT0��0-¼��������
      1-¼����ϣ�
BIT1��0-TF������
      1-TF������
BIT2��0-�������
      1-��˹���
BIT3��0-WIFI����
      1-WIFI����
BIT4��0-��������
      1-��������
BIT5��0-ң�ذ�������
      1-ң�ذ�������
BIT6��1-ͼ���˶���ⱨ��
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
����Ӧ������

1,
6.26 �洢��ý�����ݼ���Ӧ��
��ϢID��0x0802��
�洢��ý�����ݼ���Ӧ����Ϣ�����ݸ�ʽ����86��
��86 �洢��ý�����ݼ���Ӧ����Ϣ�����ݸ�ʽ
��ʼ�ֽ�	�ֶ�	��������	������Ҫ��
0	Ӧ����ˮ��	WORD 	��Ӧ�Ķ�ý�����ݼ�����Ϣ����ˮ��
2	��ý������������	WORD 	������������Ķ�ý������������
4	������		��ý����������ݸ�ʽ����87

��87 ��ý����������ݸ�ʽ
��ʼ�ֽ�	�ֶ�	��������	������Ҫ��
0	��ý��ID	DWORD	>0
4	��ý������	BYTE	0��ͼ��1����Ƶ��2����Ƶ
5	 ͨ��ID	BYTE	
6	�¼������	BYTE	0��ƽ̨�·�ָ�1����ʱ������2�����ٱ���������3����ײ�෭������������������
7	λ����Ϣ�㱨
(0x0200)��Ϣ��	BYTE[28]	��ʾ�����¼�Ƶ���ʼʱ�̵�λ�û�����Ϣ����

2,
����Ӧ��	0x38	����(���ǣ����ID	1	0x00��0x07
		���ݳ���	1	�R2
		����״̬	2	 
*/
typedef struct{    
    unsigned char ucResult;     
    unsigned char ucCMDWorld;//������ �̶�Ϊ0x38
    unsigned char ucKaKaSerialNumID;//������0 - 7
    unsigned char ucDataLen;
    unsigned short usKaKaStatu;
    //sKaKaStatus  mKaKaStatu;
}sKaKaAckBag;

sKaKaAckBag g_KakaAckBag;


/*
��ʼ�ֽ�	�ֶ�	��������	������Ҫ��
0	��Ϣ�ܰ���	WORD	����Ϣ�ְ�����ܰ���
2	�����	WORD	��1��ʼ
*/
#pragma pack(1)
typedef struct{
    unsigned short usMsgTotalPackageNm; 
    unsigned short usMsgPackIndex;
} sBB_MsgPackageItem;              //��Ϣ����װ�� 4�ֽ�
#pragma pack()

sBB_MsgPackageItem g_sBBMsgPackageItems;

//��Ϣͷ
#pragma pack(1)
typedef struct {
	unsigned short   usMsgID ;            //��ϢID u16;
	sBB_MsgBodyAttr  sMsgBodyAttr;        //��Ϣ������
	unsigned char    ucTerminalPhoneNm[6];//�ն��ֻ���
	unsigned short   usMsgSerialNm;       //��Ϣ��ˮ��
	sBB_MsgPackageItem    sMsgPackItem;   //��Ϣ����װ��,���а�������������
	int iMsgHeadLen;                      //��Ϣͷ���� ��������
} sBBMsgHeaderPack;
#pragma pack()

sBBMsgHeaderPack g_sBBMsgHeadPack,g_sBBMsgAckHeadPack;

typedef struct {
	int iMsgLen; 
    unsigned char *ucMsgArr;
}sBBMsgArr;



/*
0	Ӧ����ˮ��	WORD	��Ӧ��ƽ̨��Ϣ����ˮ��
2	Ӧ��ID	WORD	��Ӧ��ƽ̨��Ϣ��ID
4	���	BYTE	0���ɹ�/ȷ�ϣ�1��ʧ�ܣ�2����Ϣ����3����֧�֣�
*/
typedef struct{
    unsigned short usResponseSerialNm; 
    unsigned short usResponseID;
    unsigned char  ucResponseResult;
}sResponseBag;

sResponseBag g_sResponsePack;

int AnalysisMsgHard(char *pSrcBuf);


/*
ʵʱ����Ƶ�������������Ľ���
��ʼ�ֽ�	�ֶ� 	�������� 	������Ҫ�� 
0 	������IP��ַ���� 	BYTE 	����n 
1 	������IP��ַ 	STRING 	ʵʱ����Ƶ������IP��ַ 
1+n 	����������Ƶͨ�������˿ںţ�TCP��	WORD 	ʵʱ����Ƶ�������˿ں� 
3+n 	����������Ƶͨ�������˿ںţ�UDP��	WORD 	ʵʱ����Ƶ�������˿ں� 
5+n 	����Ƶͨ���� 	BYTE 	��1��ʼ 
6+n 	�Ƿ�Я����Ƶ 	BYTE 	0:Я����1:��Я�� 
7+n 	��/��������־ 	BYTE 	0:��������1:������ 
8+n 	����/�رձ�־ 	BYTE 	0:�رգ�1:���� 
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
����Ƶʵʱ����������ݸ�ʽ
��ʼ�ֽ�	�ֶ� 	�������� 	������Ҫ�� 
0 	�߼�ͨ���� 	BYTE 	2��������ǰ�� 
1 	����ָ��	BYTE 	0���ر�����Ƶ����
1���л�����
2����ͣ��ͨ���������ķ��ͣ�
3���ָ���ͣǰ���ķ��ͣ�����ͣǰ��������һ�£�
4���ر�˫��Խ�
127:����
2	�ر�����Ƶ����	BYTE	0���رո�ͨ���йص�����Ƶ���ݣ�
1��ֻ�رո�ͨ���йص���Ƶ��
2���رո�ͨ���йص���Ƶ��
3	�л���������	BYTE	��Ƶ���л�ǰ����һ�£��������������
0����������
1��������
*/
typedef struct{    
    unsigned char  ucLogicChanlNm;
    unsigned char  ucControlID;       
    unsigned char  ucCloseAVType;         //�ر�����Ƶ���� 
    unsigned char  ucSwitchStreamType;    //�л���������
}s_RealTimeAVControl;

s_RealTimeAVControl g_sRealTimeAVControl;

/*
extended protocol
ʶ����	�汾��	��Э���	����������	������
		����	��
��		
0x01	0x01	00	0x05		Уʱ
*/
typedef struct{
    unsigned char  ucIDCode;               //ʶ����
    unsigned char  ucVersionNm;            //�汾��
    //unsigned short usSubProtocolNm;
    unsigned char  ucSubProtocolFunction;  //����
    unsigned char  ucSubProtocolCommond;   //����
    unsigned short usDataLen;              //����������
    unsigned char  ucEyeExtendData[1024];  //���� 

}s_EyeExtendedProtocol;

s_EyeExtendedProtocol g_sEyeExtendedProtocolBag;

/*
����ʵʱ��Ϣ
1	ʱ��	BCD[6]	BCD���ʾ������ʱ����
2	��ƿ��ѹ	WORD	��λ��mV; ���ֽ���ǰ
3	������ת��	WORD	��λ��rpm
4	����	BYTE[1]	��λ��km/h
5	�����ſ���	BYTE[1]	��λ��1%
6	����������	BYTE[1]	��λ��1%
7	��ȴҺ�¶�	BYTE[1]	��λ����
8	˲ʱ�ͺ�	WORD	������Ϊ��ʱ����ʾ�����ͺģ���λ��L/H�������ٴ�����ʱ����ʾ�ٹ����ͺģ���λ��0.001L/100KM.<30L/100KM
9	ƽ���ͺ�	WORD	��λ: 0.001L/100KM.<30L/100KM
10	������ʻ���	DWORD	��λ����
11	�����	DWORD	��λ����
12	���κ�����	DWORD	��λ������
13	�ۼƺ�����	DWORD	��λ������
*/
typedef struct{
    unsigned char  ucTime[6]; 
    unsigned short usBatteryVoltage;

    unsigned short usEngineSpeed; 
    unsigned char  ucCarSpeed;
    unsigned char  ucThrottlePercentage;//�����ſ���
    unsigned char  ucEngineLoad;
    unsigned char  ucCoolantTemperature;//��ȴҺ�¶�

    unsigned short usInstantaneousFuelConsumption;//˲ʱ�ͺ�
    unsigned short usAverageFuelConsumption;      //ƽ���ͺ�

    unsigned short usTheMileage;                  //������ʻ��̣�
    unsigned short usTotalMileage;
    unsigned short usTheFuelConsumption;          //�����ͺ�
    unsigned short usTotalFuelConsumption;        //���ͺ�
    
}s_CarRealtimeInformation;

s_CarRealtimeInformation g_sCarRealtimeInformation;

/*������� Ҫ���г���¼���Լ켰Ӧ��
������	���ݰ�����	���ݳ���	ȡֵ
0x38	����(���ǣ����ID	1	0x00��0x07
	���ݳ���	1	�R2
	����״̬	2	BIT0��0-ACC OFF��
      1-ACC ON��
BIT1��0-���Ź�
      1-���ſ�
BIT1��0-δ����
      1-����
*/
typedef struct{
    unsigned char  ucCmdKey;                  //������
    unsigned char  ucPeripheralNmID;          //������
    unsigned char  uDataLen; 
    unsigned short usCarStatus;               //����״̬
}s_KaKaRalation;

s_KaKaRalation g_sKaKaRalationBag;

typedef struct{
    unsigned char  ucCmdKey;                  //������
    unsigned char  ucPeripheralNmID;          //������
    unsigned char  uDataLen; 
    unsigned short usKaKaStatus;              //����״̬

}s_KaKaRalationResponse;

s_KaKaRalationResponse g_sKaKaRalationResponseBag;


/*
����Э��Ĵ����״̬
*/
typedef enum
{
 STATE_USER_IDLE,	  //״̬������
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



