#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <linux/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

//cdr_daemon.c
//restart cdr_app when cdr_app crash

int main(void)
{
	int ret;
	int count = 0;
	while(1)
    {
		printf("============cdr_app Start !=================\r\n");	
        system("killall -9 cdr_app");
        ret = system("/app/cdr_app");
		printf("cdr_app has exit ! the return value is %d\r\n", ret);
		if((-1 == ret) || (WEXITSTATUS(ret) != 0xa))
        {				
			printf("Non-normal exit the  !\r\n");
		}

		//守护进程如果两百次退出，就会恢复出厂设置;
		//因为key reset 在cdr_app 里面.这样就没法恢复出厂设置了..
		count ++;
		if(count == 200)
		{
			system("cp -rf /usr/cdr_syscfg.xml /home/");
			system("cp -rf /usr/cdr_syscfg.xml /mnt/cfg/");
			system("sync");
			sleep(1);
			system("reboot");    
		}
        printf(" will restart !\r\n");
	}
}
