#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include<time.h>  //getSystemTimer
#include<sys/time.h>
#include <sys/timeb.h>

#include <signal.h>
#include <sys/select.h>

char*    sample_discript[6] = {"端口3 串口5测试结束", "端口3 串口3测试结束", "端口4 串口2测试结束", "端口4 串口3测试结束", "端口5 485-1 485-2测试结束", "端口2-485 <-->端口3-485 测试结束"} ;
char*     end_of_test[8] = {"×", "×", "×", "×", "×", "×", "×", "×"} ;

void fork_entry(char* cmd, int port, int discript)
{
	int fd = 0 ;
	int ret = 0 ;
	char  buf[8] = {0};
	
	system(cmd) ;
	
	fd = open("test_log.txt", O_RDONLY) ;
	ret = read(fd, buf, sizeof(buf)) ;
	
	if(buf[0] == '0')
	{
		end_of_test[discript] = "√" ;
printf("\n************\n\
* 串口%d功能测试成功 %s\n\
************\n\
", port, sample_discript[discript]) ;
	}
	else
	{
		if(buf[2] == 'e')
			printf("\n--------——————----串口%d无法通信 请检查电路 %s-------——————--------\n",port, sample_discript[discript]) ;
			
		if(buf[2] == '1')
			printf("\n******串口%d发送测试失败 %s******\n", port, sample_discript[discript]) ;
		if(buf[4] == '1')
			printf("\n******串口%d接收测试失败 %s******\n", port, sample_discript[discript]) ;
		
	}
	
	close(fd) ;
	
}
int main(int argc, char* argv[])
{
	int fd = 0 ;
	int ret = 0 ;
	char  buf[6] = {0};
	
	
	int bootdelay  = 5 ;

	system("echo  \"AT\QCRMCALL=1,1\" > /dev/ttyUSB3") ;
	system("udhcpc -i wwan0 -n -q") ;
	//printf("<<<<<<<<<<<<<<<<<<<<<<<<<< 准备测试串口 >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n") ;
	printf("\n                            倒计时：  %2ds", bootdelay) ;
	
	while(bootdelay > 0)
	{
		sleep(1);
		bootdelay-- ;
		printf("\b\b\b\b%2ds ", bootdelay);
		fflush(stdout);
		
	}
	
	system("rm  show.txt  -f  && sync") ;
	
	if(argc > 1)
	{
		if(strcmp(argv[1], "-show")==0)  
			system("touch  show.txt  && chmod 666 show.txt") ;

	}
	
	fork_entry("aotu_test  /dev/ttyO5  lo -d", 5, 0) ;
	fork_entry("aotu_test  /dev/ttyO3  lo -d", 3, 1) ;
	fork_entry("aotu_test  /dev/ttysWZA2 lo -d", 2, 2) ;
	fork_entry("aotu_test  /dev/ttysWZA3 lo -d", 3, 3) ;
	fork_entry("aotu_test  /dev/ttysWZA0 /dev/ttysWZA1 -d  -485", 12, 4) ;
	fork_entry("aotu_test  /dev/ttyO4   /dev/ttyO5     -d  -485", 23, 5) ;
    //can   
	system("can_aotu -s can0 -t &") ;
	sleep(1) ;
	system("can_aotu -r can1 -d") ;
	
	fd = open("d_can_log.txt", O_RDONLY) ;
	ret = read(fd, buf, sizeof(buf)) ;
	if(buf[0] == '0')
	{
		end_of_test[6] = "√" ;
printf("\n\n************\n\
* can0<-->can1 功能测试成q功 \n\
************\n\
") ;
	}
	else
	{
		if(buf[0] == 'e')
			printf("\n******can0<-->can1 无法通信 请检查电路 %s******\n") ;
		else
			printf("\n******can0<-->can1 测试失败 %s******\n") ;
		
	}
	//4G
	ret = system("ping 8.8.8.8 -c2") ;
	
	if(ret == 0)
	{
		end_of_test[7] = "√" ;
printf("\n\n************\n\
* 4G模块测试成功 \n\
************\n\
") ;
	}
	else
printf("\n\n************\n\
* 4G模块测试失败 \n\
************\n\
") ;
   //输出最终结果
	printf("\n*****************************\n\
*  端口3 串口5              %s\n\
*  端口3 串口3              %s\n\
*  端口4 串口2              %s\n\
*  端口4 串口3              %s\n\
*  端口5 485-1  485-2       %s\n\
*  端口2-485 <-->端口3-485  %s\n\
*  端口1 can0<-->can1       %s\n\
*  4G模块上网功能           %s\n\
*****************************\n\
", end_of_test[0], end_of_test[1], end_of_test[2], end_of_test[3], end_of_test[4], end_of_test[5], end_of_test[6], end_of_test[7]) ;
	
	close(fd) ;
	
	return 0 ;
}