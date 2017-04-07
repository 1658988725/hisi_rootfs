#include <string.h>
#include "queue_bag.h"

//缓存接收包队列
unsigned char ucRecvBagQueueBuf[RECVBAG_QUEUE_BUF_SIZE];
BagQueue stRecvBagQueue;

//缓存返回包队列
unsigned char ucRetuBagQueueBuf[RETUBAG_QUEUE_BUF_SIZE];
BagQueue stRetuBagQueue;


//--------------------------------------------------------------
void RecvBagQueueInit(BagQueue *stQueue)
{
	stQueue->pucBuf   = &ucRecvBagQueueBuf[0];
	stQueue->usHead   = 0;	
	stQueue->usTail   = 0;
	memset(stQueue->pucBuf, 0, RECVBAG_QUEUE_BUF_SIZE);
}
//--------------------------------------------------------------
void RetuBagQueueInit(BagQueue *stQueue)
{
	stQueue->pucBuf   = &ucRetuBagQueueBuf[0];
	stQueue->usHead   = 0;	
	stQueue->usTail   = 0;
	memset(stQueue->pucBuf, 0, RETUBAG_QUEUE_BUF_SIZE);
}

//--------------------------------------------------------------
unsigned char BagQueueAddData(BagQueue *stQueue, unsigned char *pucBag, unsigned short usBagLen)
{    
    //printf("stQueue->usTail :%d\n",stQueue->usTail);
    	memcpy(stQueue->pucBuf + stQueue->usTail * MAX_BUFF_BAG_LEN, pucBag, usBagLen);

    stQueue->usBagLen[stQueue->usTail] = usBagLen;
	
	if(stQueue->usTail == MAX_BAG_NUM - 1)
	{
		stQueue->usTail = 0;	
	}else{
		stQueue->usTail = stQueue->usTail+1;
	}	
	return 1;	
}

//--------------------------------------------------------------
int BagQueueGetData(BagQueue *stQueue,unsigned char **pucBag)
{
    int iBagLen = 0;

	if(stQueue->usHead == stQueue->usTail) return 0; 
    
	*pucBag = stQueue->pucBuf + stQueue->usHead * MAX_BUFF_BAG_LEN;

    iBagLen = stQueue->usBagLen[stQueue->usHead];
	
	if(stQueue->usHead == MAX_BAG_NUM - 1)
	{
		stQueue->usHead = 0;
	}else{
		stQueue->usHead = stQueue->usHead + 1;
	}

	return iBagLen;
}

//--------------------------------------------------------------
unsigned char BagQueueCheckEmpty(BagQueue *stQueue)
{
	if(stQueue->usHead == stQueue->usTail)
	{
         //printf("当前队列为空!\n");
		return 1;
	}	
	return 0;
}
