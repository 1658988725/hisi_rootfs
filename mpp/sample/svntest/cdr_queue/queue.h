#if(0)
#ifdef             __QUEUE__



#ifndef RT_DEBUG_PROTO
#define RT_DEBUG_PROTO        1
#endif

typedef struct
{
	unsigned char *pucBuf;
	unsigned short usHead;
	unsigned short usTail;
	unsigned short usBufSize;
}Queue;


extern Queue stUDPQueue;

void UDPRecvQueueInit(Queue *stQueue);
unsigned char QueueAddData(Queue *stQueue, unsigned char ucData);
unsigned char QueueGetData(Queue *stQueue, unsigned char *pData);
unsigned char QueueCheckEmpty(Queue *stQueue);
unsigned short QueueGetCount(Queue *stQueue);

#endif

#endif

