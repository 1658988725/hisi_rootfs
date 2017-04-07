#ifndef QUEUE_BAG_H
#define QUEUE_BAG_H

#include"cdr_bubiao_analyze.h"

#define              MAX_BAG_NUM   	                         10

#define              MAX_RETUBAG_LEN		                 BUBIAO_MAX_LEN//1024//120 //406�������չ���������
#define              MAX_RECVBAG_LEN                         BUBIAO_MAX_LEN//100
#define              MAX_BUFF_BAG_LEN                        BUBIAO_MAX_LEN//������ÿ�����̶�ռ�еĳ���

#define              RETUBAG_QUEUE_BUF_SIZE                  (MAX_BAG_NUM * MAX_RETUBAG_LEN)
#define              RECVBAG_QUEUE_BUF_SIZE                  (MAX_BAG_NUM * MAX_RECVBAG_LEN)

typedef struct
{
	unsigned char *pucBuf;
	unsigned short usBagLen[MAX_BAG_NUM];//�洢ÿ����ʵ�ʳ���
	unsigned short usHead;
	unsigned short usTail;
}BagQueue;

extern BagQueue stRecvBagQueue;
extern BagQueue stRetuBagQueue;

void RecvBagQueueInit(BagQueue *stQueue);
void RetuBagQueueInit(BagQueue *stQueue);

unsigned char BagQueueAddData(BagQueue *stQueue, unsigned char *pucBag, unsigned short usBagLen);
int BagQueueGetData(BagQueue *stQueue,unsigned char **pucBag);
unsigned char BagQueueCheckEmpty(BagQueue *stQueue);

#endif

