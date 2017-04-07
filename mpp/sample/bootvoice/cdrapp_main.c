/******************************************************************************
  A simple program of Hisilicon HI3531 video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include "cdr_audio.h"
#include "cdr_config.h"

int playflag = 1;
int main(int argc, char *argv[])
{		
	cdr_config_init();
	cdr_audioInit();
	if(g_cdr_systemconfig.bootVoice)
	cdr_audio_play_file("/home/audiofile/poweron.pcm");		
	while(playflag) usleep(100);	
	cdr_audio_release();	
	return 0;
}

