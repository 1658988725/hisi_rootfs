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
����Э���ת�廹ԭ -->�����յ�������sBBMsgArrPack��ԭ��ԭ����ʵ������
7d -> 7d 01   7e ->7d 02
pSrcBuf:ת��ǰ�����ݣ� �������
pERDstBag:ת�������� �������
iBBPackLen:�������
����:ת��ǰ ԭʼ���ݵĳ��� 
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
����Э���ת�� �����ӽ�����ʶ��
pSrcBuf:ת��ǰ�����ݣ�
pERDstBag:ת��������
����:ת���ĳ���
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
У����ָ����Ϣͷ��ʼ��ͬ��һ�ֽ����ֱ��У����ǰһ���ֽڣ�ռ��һ���ֽڡ�
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
    //return -1;// 406 ����ͨ����ƽ̨����ͨ��
    return 0;
  }else{
    //printf("bubiao check code success!\n");
    return 0;
  }
}

