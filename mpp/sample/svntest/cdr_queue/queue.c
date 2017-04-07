#if(0)
#define                 __QUEUE__

#include <string.h>
#include "queue.h"

Queue stUDPQueue;

//串口接收队列
#define              UDP_QUEUE_BUF_SIZE                    300//128

unsigned char ucUDPQueueBuf[UDP_QUEUE_BUF_SIZE];


//--------------------------------------------------------------
unsigned char QueueAddData(Queue *stQueue, unsigned char ucData)
{
	stQueue->pucBuf[stQueue->usTail] = ucData;
			
	if(stQueue->usTail == stQueue->usBufSize - 1)
	{
		stQueue->usTail = 0;	
	}
	else
	{
		stQueue->usTail = stQueue->usTail+1;
	}
	
	return 1;	
}
//--------------------------------------------------------------
unsigned char QueueGetData(Queue *stQueue,unsigned char *pData)
{
	if(stQueue->usHead == stQueue->usTail)
	{
		return 0; 
	}
	
	*pData = stQueue->pucBuf[stQueue->usHead];
	
	if(stQueue->usHead == stQueue->usBufSize - 1)
	{
		stQueue->usHead = 0;
	}
	else
	{
		stQueue->usHead = stQueue->usHead + 1;
	}

	return 1;
}
//--------------------------------------------------------------
unsigned char QueueCheckEmpty(Queue *stQueue)
{
	if(stQueue->usHead == stQueue->usTail)
	{
		return 1;
	}	
	return 0;
}

//--------------------------------------------------------------
void UDPRecvQueueInit(Queue *stQueue)
{    
    stQueue->pucBuf = &ucUDPQueueBuf[0];

	stQueue->usHead  = 0;	
	stQueue->usTail  = 0;
	stQueue->usBufSize = UDP_QUEUE_BUF_SIZE;
	memset(stQueue->pucBuf, 0, stQueue->usBufSize);
}
#endif

