
#ifndef BUDIAO_H
#define BUDIAO_H

#include "cdr_count_file.h"
#include "cdr_bubiao_analyze.h"


extern unsigned short usResponseSerialNm;
void InitAckBag();

/*
�洢ý���Ҫ��Ϣ��ѯӦ��
��Ӧ����ˮ��	WORD	
����ʱ��	BCD[6]	����ѯ���͵��ļ�������ʱ��
���ʱ��	BCD[6]	����ѯ���͵��ļ������ʱ��
�ļ�����	BYTE	����ѯ���͵��ļ�������
*/
typedef struct{
    unsigned char  ucMediaType;
    unsigned char  ucChnID;
    unsigned char  ucStartTime[15]; //BCD[6] 14 15  ����bcd 6�����bug ,1970 ��ռ��2���ֽ�
    unsigned char  ucEndTime[15]; 
    unsigned short usFileCnt;
}s_MediaProfileInformBody;

s_MediaProfileInformBody g_sMediaProfileBody;
int MediaProfileInformQueryProcess();


/*
��85 �洢��ý�����ݼ�����Ϣ�����ݸ�ʽ
��ʼ�ֽ�	�ֶ�	��������	������Ҫ��
0	��ý������	BYTE	0��ͼ��1����Ƶ��2����Ƶ��
1	ͨ��ID	BYTE	0 ��ʾ������ý�����͵�����ͨ����
2	�¼������	BYTE	0��ƽ̨�·�ָ�1����ʱ������2�����ٱ���������3����ײ�෭������������������
3	��ʼʱ��	BCD[6]	YY-MM-DD-hh-mm-ss
9	����ʱ��	BCD[6]	YY-MM-DD-hh-mm-ss
*/
typedef struct{
    unsigned char  ucMediaType;  
    unsigned char  ucChnID;      
    unsigned char  uEventItemCode;
    unsigned char  ucStartTime[15]; //BCD[6] 14
    unsigned char  ucEndTime[15]; 
    unsigned short usMediaBagCnt;
    unsigned short usMediaDataTatoil;  //��ý������������  2017 03 02����
    unsigned short usPreBagMediaDataTatoil;  //ÿ�����ذ���ý������������  2017 03 02����

    unsigned char  ucFileName[100];
    unsigned short usVideoLen;  
}s_DataSearchMsgBody;

s_DataSearchMsgBody g_sDataSearchMsgBag;
unsigned short  g_usMediaDataTatoil;  //��ý������������


int MediaDataInformQueryProcess();

/*
����	BYTE	0��ֹͣ���أ�1����ʼ���� 
�ļ�������	BYTE	
�ļ���	BYTE[��]	�ļ���ASCII�ֽ����飬������չ��
�ļ�����	BYTE	1����ͼ��2����ͼ����ͼ��3��������Ƶ��4��������ƵԤ��ͼ��һ�ţ�
�ϴ�URL����	BYTE	
�ϴ�URL	BYTE[��]	
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
����	BYTE	0��ֹͣ�ϴ���1����ʼ�ϴ�
��ʼʱ���	BCD[6]	
ʱ��	WORD	��λ����
�ļ�����	BYTE	1��ѭ����Ƶ��2��ѭ����ƵԤ��ͼ
�ϴ�URL����	BYTE	
�ϴ�URL	BYTE[��]	
*/
typedef struct{
    unsigned char  ucUploadActive;  
    unsigned char  ucChnID;                 
    unsigned char  ucStartTime[15]; //BCD[6] 17-01-23 15-17-59 (6���ֽ�)���ֶν��յ�������Ҫ����ת�����20170221 121350 (15���ֽ�)��ʽ
    unsigned short  usAVTimeLen;
    unsigned char  ucEndTime[15]; //����
    unsigned char  ucFileType;
    unsigned char  ucUploadURLLen;
    unsigned char  ucUploadURL[255]; 

    unsigned char  ucResult;
    char sAreaMediaUploadFileName[100];//�������ý���ϴ���Ӧ���ļ���
}s_AreaMediaUploadAttr;

s_AreaMediaUploadAttr g_sAreaMediaUploadAttrBody;
int AreaMediaUploadQueryProcess();


/*
��ʼ�ֽ�	�ֶ�	��������	������Ҫ��
0	�߼�ͨ����	BYTE	����JT/T 1076-2016�еı�2.0��ʾ����ͨ��
1	��ʼʱ��	BCD[6]	YY-MM-DD-HH-MM-SS��ȫ0��ʾ����ʼʱ������
7	����ʱ��	BCD[6]	YY-MM-DD-HH-MM-SS��ȫ0��ʾ����ֹʱ������
13	������־	64BITS	������־λ�������24
ȫ0��ʾ��ָ���Ƿ��б���
21	����Ƶ��Դ����	BYTE	0������Ƶ��1����Ƶ��2����Ƶ��3����Ƶ��������Ƶ
22	��������	BYTE	0������������1����������2��������
23	�洢������	BYTE	0�����д洢����1�����洢����2���ֱ��洢��
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
������IP��ַ���� 	BYTE 	����n 
������IP��ַ 	STRING 	ʵʱ����Ƶ������IP��ַ 
����������Ƶͨ�������˿ںţ�TCP�� 	WORD 	ʵʱ����Ƶ�������˿ں�,������TCP����ʱ��0
����������Ƶͨ�������˿ںţ�UDP�� 	WORD 	ʵʱ����Ƶ�������˿ں�,������UDP����ʱ��0
�߼�ͨ���� 	BYTE 	2��������ǰ�� 
����Ƶ����	BYTE 	0������Ƶ��1����Ƶ��2����Ƶ��3����Ƶ������Ƶ
�������� 	BYTE 	0������������������1����������2���������������ͨ��ֻ������Ƶ�����ֶ���0 
�طŷ�ʽ 	BYTE 	0�������طţ�
1������طţ� 
2���ؼ�֡���˻طţ�
3���ؼ�֡���ţ�  
4����֡�ϴ� 
�������˱��� 	BYTE 	�طŷ�ʽΪ1��2ʱ�����ֶ�������Ч��������0�� 
0����Ч��
1��1����
2��2����
3��4����
4��8����
5��16��
��ʼʱ�� 	BCD[6] 	YY--DD-HH-NN-SS���طŷ�ʽΪ4ʱ�����ֶα�ʾ��֡�ϴ�ʱ�� 
����ʱ�� 	BCD[6] 	YY-MM-DD-HH-NN-SS��Ϊ0��ʾһֱ�طţ��طŷ�ʽΪ4ʱ�����ֶ���Ч
*/
typedef struct{
    unsigned char  ucServerIPAddrLen; //������IP��ַ����
    char  usServerIPAddr[200];        //192.168.166.120 -15 \0 url  ʵʱ����Ƶ������IP��ַ
    unsigned short usServerTCPListenPort;
    unsigned short usServerUDPListenPort;
    unsigned char  ucAVChanlNm;          //2��������ǰ��
    unsigned char  ucMediumType;         //0x00 :vodio 0x01:audio 0x02:picture
    unsigned char  ucStreamType;    
    unsigned char  ucPlayBackWay;        //�طŷ�ʽ
    unsigned char  ucSpeedNum;           //������˱���
    
    unsigned char  ucStartTime[15];
    unsigned char  ucEndTime[15];     
}s_VideoPlayBackBody;    



/*
����Ƶͨ����	BYTE	����JT/T 1076-2106�еı�2
�طſ���	BYTE	0����ʼ�ط�
1����ͣ�ط�
2�������ط�
3������ط�
4���ؼ�֡���˻ط�
5���϶��ط�
6���ؼ�֡����
�������˱���	BYTE	�طſ���Ϊ3��4ʱ�����ֶ�������Ч��������0��
0����Ч��
1:1����
2:2����
3:4����
4:8����
5:16����
�϶��ط�λ��	BCD[6]	YY-MM-DD-HH-MM-SS���طſ���Ϊ5ʱ�����ֶ���Ч
*/
typedef struct{
    unsigned char  ucAVChnID; 
    unsigned char  ucPlayBackControl;            //0����ʼ�ط�1����ͣ�ط�2...
    unsigned char  ucSpeedNum;                   //������˱���    
    unsigned char  ucPlayBackStartTime[15];      //�϶��ط�λ��
}s_VideoPlayBackControlBody; 

/*
�����ѯ�Ľ���洢�Ľṹ��
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



