
#ifndef COUNT_FILE_H  
#define COUNT_FILE_H

#define PER_BAG_MAX_ITEMS_CNT        5      //每个返回的应答包，包含 的 多媒体检索项数据 最多个数

/*
消息包封装项内容
*/
typedef struct{
    int iTotailBagCnt;//消息总包数
    int iBagIndex;    //包序号 start 1
}s_MsgPackagItems;

s_MsgPackagItems g_sMsgPackagItems;

/*
存储所有媒体检索项 属性
检索项列表

若为图片假定一个设备中最多存有图片的的上限为1000张
*/
typedef struct{
    int iIndex; 
    
    char cLatitudeLongitudeFlag; //0: 北纬东经，1:西经南纬  协议定义不好
    char cLatituFlag;            //0:东经 1:西经
    //double dlatitude;            //经度 double 范围类需要修改，暂时不细究
    float dlatitude;            //经度 double 范围类需要修改，暂时不细究
    char cLongFlag;              //0:北纬 1:南纬
    float dlongitude;           //纬度

    unsigned char ucFileNameLen;    
    unsigned char ucFullFileNameBuf[45];    //文件名 全称
    unsigned int  uiFileSize;
    unsigned char  ucFileTime[15];
    unsigned short usVideoLen;    
    /*   扩展信息长度	BYTE	目前未定义，0字节   扩展信息	BYTE[…]	    */
}sSearchTotailMediaItemBuf;

sSearchTotailMediaItemBuf g_sSearchTotailPictBuf[PER_BAG_MAX_ITEMS_CNT];//存储每个返回包查询到的媒体



/*
0	流水号	WORD	对应查询音视频资源列表指令的流水号
2	音视频资源总数	DWORD	无符号条件的音视频资源，置为0
6	音视频资源列表		见表23

表23 终端上传音视频资源列表格式
起始字节	字段	数据类型	描述及要求
0	逻辑通道号	BYTE	按照JT/T 1076-2106中的表2，2：车辆正前方
1	开始时间	BCD[6]	YY-MM-DD-HH-MM-SS
7	结束时间	BCD[6]	YY-MM-DD-HH-MM-SS
13	报警标志	64BITS	报警标志位定义见表24
全0表示不指定是否有报警
21	音视频资源类型	BYTE	0：音视频，1：音频，2：视频
22	码流类型	BYTE	1：主码流，2：子码流
23	存储器类型	BYTE	1：主存储器，2：灾备存储器
24	文件大小	DWORD	单位字节（BYTE）*/
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

sSearchSourceListItemBuf g_sSearchSourceBuf[PER_BAG_MAX_ITEMS_CNT];//存储每个返回包查询到的媒体



int PaserCountFileNumber(char *pDir);

int compare_time(unsigned char *pTimeStart,unsigned char *pTm2,unsigned char *pTimeEnd);


#endif  

