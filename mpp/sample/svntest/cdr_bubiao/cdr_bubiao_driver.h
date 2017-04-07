
#ifndef BUDIAO_DRIVER_H
#define BUDIAO_DRIVER_H

#include "cdr_bubiao.h"
#include "cdr_bubiao_analyze.h"
#include "cdr_ftp_process.h"

int EscapeProcess(unsigned char *pSrcBuf,unsigned char *pEDstBag,int iBBSrcPackLen);
int BBCheckCodeProcess(sBBMsgArr sBBMsgArrPack);
void BBEscapeReduction(sBBMsgArr *sBBMsgArrPack);

unsigned char GetCheckCode(unsigned  char *pSrcBuf,int iPackLen);
int CheckCodeProcess(char *pSrcBuf,int iPackLen);

#endif



