
#ifndef COUNT_FILE_H  
#define COUNT_FILE_H

#define PER_BAG_MAX_ITEMS_CNT        5      //ÿ�����ص�Ӧ��������� �� ��ý����������� ������

/*
��Ϣ����װ������
*/
typedef struct{
    int iTotailBagCnt;//��Ϣ�ܰ���
    int iBagIndex;    //����� start 1
}s_MsgPackagItems;

s_MsgPackagItems g_sMsgPackagItems;

/*
�洢����ý������� ����
�������б�

��ΪͼƬ�ٶ�һ���豸��������ͼƬ�ĵ�����Ϊ1000��
*/
typedef struct{
    int iIndex; 
    
    char cLatitudeLongitudeFlag; //0: ��γ������1:������γ  Э�鶨�岻��
    char cLatituFlag;            //0:���� 1:����
    //double dlatitude;            //���� double ��Χ����Ҫ�޸ģ���ʱ��ϸ��
    float dlatitude;            //���� double ��Χ����Ҫ�޸ģ���ʱ��ϸ��
    char cLongFlag;              //0:��γ 1:��γ
    float dlongitude;           //γ��

    unsigned char ucFileNameLen;    
    unsigned char ucFullFileNameBuf[45];    //�ļ��� ȫ��
    unsigned int  uiFileSize;
    unsigned char  ucFileTime[15];
    unsigned short usVideoLen;    
    /*   ��չ��Ϣ����	BYTE	Ŀǰδ���壬0�ֽ�   ��չ��Ϣ	BYTE[��]	    */
}sSearchTotailMediaItemBuf;

sSearchTotailMediaItemBuf g_sSearchTotailPictBuf[PER_BAG_MAX_ITEMS_CNT];//�洢ÿ�����ذ���ѯ����ý��



/*
0	��ˮ��	WORD	��Ӧ��ѯ����Ƶ��Դ�б�ָ�����ˮ��
2	����Ƶ��Դ����	DWORD	�޷�������������Ƶ��Դ����Ϊ0
6	����Ƶ��Դ�б�		����23

��23 �ն��ϴ�����Ƶ��Դ�б��ʽ
��ʼ�ֽ�	�ֶ�	��������	������Ҫ��
0	�߼�ͨ����	BYTE	����JT/T 1076-2106�еı�2��2��������ǰ��
1	��ʼʱ��	BCD[6]	YY-MM-DD-HH-MM-SS
7	����ʱ��	BCD[6]	YY-MM-DD-HH-MM-SS
13	������־	64BITS	������־λ�������24
ȫ0��ʾ��ָ���Ƿ��б���
21	����Ƶ��Դ����	BYTE	0������Ƶ��1����Ƶ��2����Ƶ
22	��������	BYTE	1����������2��������
23	�洢������	BYTE	1�����洢����2���ֱ��洢��
24	�ļ���С	DWORD	��λ�ֽڣ�BYTE��*/
typedef struct{
    unsigned char ucLogicChnID;    

    unsigned char ucFileStartTime[15];    
    unsigned char ucFileEndTime[15];
    
    unsigned char ucWarningMark[8];
    unsigned char  ucSourceType;
    unsigned char  ucStreamType;
    unsigned char  ucStoreType; 
    unsigned int  uiFileSize;    
        
}sSearchSourceListItemBuf;

sSearchSourceListItemBuf g_sSearchSourceBuf[PER_BAG_MAX_ITEMS_CNT];//�洢ÿ�����ذ���ѯ����ý��



int PaserCountFileNumber(char *pDir);

int compare_time(unsigned char *pTimeStart,unsigned char *pTm2,unsigned char *pTimeEnd);


#endif  

