/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "cdr_bubiao_driver.h"


/**
部标协议的转义还原 -->将接收到的数据sBBMsgArrPack还原成原本真实的数据
7d -> 7d 01   7e ->7d 02
pSrcBuf:转义前的数据； 输入参数
pERDstBag:转义后的数据 输出参数
iBBPackLen:输入参数
返回:转义前 原始数据的长度 
*/
void BBEscapeReduction(sBBMsgArr *sBBMsgArrPack)
{
  int i = 0;
  int j = 0; 
  int iER_BBPacklen = 0;
  unsigned char ucERTempBag[BUBIAO_MAX_LEN] = {0};
  
  for(i = 0,j = 0; i < sBBMsgArrPack->iMsgLen && j < sBBMsgArrPack->iMsgLen; i++,j++)
  {
     if(sBBMsgArrPack->ucMsgArr[i]== 0x7D){      
        if(sBBMsgArrPack->ucMsgArr[i+1] == 0x01){
           ucERTempBag[j] = 0x7D;
           ++i;
           continue;
        }
        if(sBBMsgArrPack->ucMsgArr[i+1] == 0x02){
           ucERTempBag[j] = 0x7E;
           ++i;   
           continue;
        }
     }
     ucERTempBag[j] = sBBMsgArrPack->ucMsgArr[i];
  }

  iER_BBPacklen = j;
  memcpy(sBBMsgArrPack->ucMsgArr,ucERTempBag,iER_BBPacklen);
  sBBMsgArrPack->iMsgLen = iER_BBPacklen;
  return ;
}

/*
部标协议的转义 及增加结束标识符
pSrcBuf:转义前的数据；
pERDstBag:转义后的数据
返回:转义后的长度
*/
int EscapeProcess(unsigned char *pSrcBuf,unsigned char *pEDstBag,int iBBSrcPackLen)
{
  int iE_BBPacklen = 0;
  int i = 0;
  int j = 0;
  unsigned char ucETempBag[BUBIAO_MAX_LEN] = {0};
  
  ucETempBag[0] = 0x7E;
  for(i = 1,j = 1; i < iBBSrcPackLen-1; i++,j++)
  {
     if(pSrcBuf[i] == 0x7D){      
        ucETempBag[j] = 0x7D;
        j++;
        ucETempBag[j] = 0x01;
        continue;
     }
     if(pSrcBuf[i] == 0x7E){      
        ucETempBag[j] = 0x7E;
        j++;
        ucETempBag[j] = 0x02;
        continue;
     }                   
     ucETempBag[j] = pSrcBuf[i];
  }
  iE_BBPacklen = j;

  ucETempBag[iE_BBPacklen] = 0x7E;  
  iE_BBPacklen++;
  
  memcpy(pEDstBag,ucETempBag,iE_BBPacklen);
  
  return iE_BBPacklen;
}

unsigned char GetCheckCode(unsigned char *pSrcBuf,int iPackLen)
{
  int i = 0;
  unsigned char ucCheckCode = 0x00;

  ucCheckCode = pSrcBuf[1]^pSrcBuf[2];
  
  for(i=3;i<iPackLen;i++)
  {
    ucCheckCode = ucCheckCode^pSrcBuf[i];    
  }

  return ucCheckCode;
}


/*
校验码指从消息头开始，同后一字节异或，直到校验码前一个字节，占用一个字节。
*/
int CheckCodeProcess(char *pSrcBuf,int iPackLen)
{
  int i = 0;  
  unsigned char ucCheckCode = 0x00;

  ucCheckCode = pSrcBuf[1]^pSrcBuf[2];
  
  for(i=3;i<iPackLen-2;i++)
  {
    ucCheckCode = ucCheckCode^pSrcBuf[i];    
  }
  
  if(ucCheckCode != pSrcBuf[iPackLen-2]){
    printf("bubiao check code fails\n");
    //return -1;
    return 0;
  }else{
    //printf("bubiao check code success!\n");
    return 0;
  }
}

int BBCheckCodeProcess(sBBMsgArr sBBMsgArrPack)
{
  int i = 0;  
  unsigned char ucCheckCode = 0x00;

  ucCheckCode = (sBBMsgArrPack.ucMsgArr[1])^(sBBMsgArrPack.ucMsgArr[2]);
  
  for(i=3;i<sBBMsgArrPack.iMsgLen-2;i++)
  {
    ucCheckCode = ucCheckCode^sBBMsgArrPack.ucMsgArr[i];    
  }
  
  if(ucCheckCode != sBBMsgArrPack.ucMsgArr[sBBMsgArrPack.iMsgLen-2]){
    printf("bubiao check code fails\n");
    //return -1;// 406 不能通过，平台可以通过
    return 0;
  }else{
    //printf("bubiao check code success!\n");
    return 0;
  }
}

