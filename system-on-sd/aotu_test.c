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

#define    STRING    "123456789\0"
#define    LENGHTH   10

struct DATE{
	unsigned long send_success;
	unsigned long send_fail;
	unsigned long receive_success;
	unsigned long receive_fail;
};
static struct DATE date_info = {0, 0, 0, 0} ;

static  struct termios termold_port[3],termnew_port[3];


static  int  fd[3] = {0}  ;
int loop_time = 0 ;         

int loop_mode = 0 ; //测试模式
static int test_time_out = 0 ;
int display = 0 ;

void   alarm_handler()  ;
//gettime from system
void getSystemTimer(void)
{
    struct timeb timebuffer;  //get ms
	static long  oldsecond ;  long msecond = 0 ;
 
    static char flag=0;
	static long lastsecond = 0 ;   //old
	long timeoffset = 0 ;          //now
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv,&tz);
	if(flag==0){
	  lastsecond = tv.tv_sec ;
	  flag = 1 ;
	}
	timeoffset = tv.tv_sec - lastsecond ; //get offsettime
	//ms
	ftime(&timebuffer);  
    msecond = (tv.tv_sec-oldsecond)*1000 + timebuffer.millitm ;
	oldsecond = tv.tv_sec ;
	//printf("\n*************************    %02d:%02d:%02d    **************************\n",(timeoffset/60)/60,(timeoffset/60)%60,timeoffset%60) ;  //48.8*2
	//printf("--send_success-- --send_fail-- --receive_success-- --receive_fail--\n") ;
	if(loop_mode==1 && display==0) //只有工厂测试模式指定下 才不需要显示
		;
	else
		printf("  % 6d           % 6d        % 6d              % 6d\n",date_info.send_success, date_info.send_fail, date_info.receive_success, date_info.receive_fail) ;
	//
	loop_time++ ; 
}

//print
void print_result(void)
{
	long unsigned int fail=0;
	long unsigned int all=0;
	char i=0;
	double loss_pack = 0.0 ;
	//printf("\n**************************B115200**************************\n");
	getSystemTimer() ;

}

int port_lo(char* uart1)
{
	fd[1] = open(uart1, O_RDWR ) ;
	if(fd[1] < 0)
	{
		printf("串口打开失败\n") ;
		exit(0) ;
	}
	if(tcgetattr(fd[1], &termold_port[1]) != 0)
		{
				perror("SetupSerial 1");
				exit(0);
		}
		 bzero(&termnew_port[1], sizeof(termnew_port[1]));
		  termnew_port[1].c_iflag &= ~(ICRNL|IGNCR) ;
			termnew_port[1].c_cflag |= CLOCAL | CREAD;   //CLOCAL:忽略modem控制线  CREAD：打开接受者
			termnew_port[1].c_cflag &= ~CSIZE; 
			termnew_port[1].c_cflag |= CS8;
			termnew_port[1].c_cflag &= ~PARENB;
			cfsetispeed(&termnew_port[1], B115200);
			cfsetospeed(&termnew_port[1], B115200);
			termnew_port[1].c_cflag &=  ~CSTOPB;
			termnew_port[1].c_cc[VTIME]  = 20;                    //VTIME:非cannoical模式读时的延时，以十分之一秒位单位
			termnew_port[1].c_cc[VMIN] = LENGHTH ;                       //VMIN:非canonical模式读到最小字符数
			tcflush(fd[1],TCIFLUSH);                         // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
			if((tcsetattr(fd[1], TCSANOW,&termnew_port[1]))!=0)   //TCSANOW:改变立即发生
			{
				perror("com set error");
				return -1;
			}
			//perror(" set done!");	
	fd[2] = fd[1] ;
	
	return 0 ;

}
int ports_cfg(char* uart1,char* uart2)
{
	int i = 0  ;
	
	if(strcmp(uart2, "lo") == 0)
	{
		port_lo(uart1) ;
		return 0 ;
	}
	fd[1] = open(uart1, O_RDWR ) ;
	fd[2] = open(uart2, O_RDWR ) ;

	if(fd[1] < 0 || fd[2] < 0)
	{
		printf("串口打开失败 请单独测试\n") ;
		exit(0) ;
	}
	for(i=1; i<=2; i++)
	{
		if(tcgetattr(fd[i], &termold_port[i]) != 0)
		{
				perror("SetupSerial 1");
				return -1;
		}
		 bzero(&termnew_port[i], sizeof(termnew_port[i]));
		  termnew_port[i].c_iflag &= ~(ICRNL|IGNCR) ;
			termnew_port[i].c_cflag |= CLOCAL | CREAD;   //CLOCAL:忽略modem控制线  CREAD：打开接受者
			termnew_port[i].c_cflag &= ~CSIZE; 
			termnew_port[i].c_cflag |= CS8;
			termnew_port[i].c_cflag &= ~PARENB;
			cfsetispeed(&termnew_port[i], B115200);
			cfsetospeed(&termnew_port[i], B115200);
			termnew_port[i].c_cflag &=  ~CSTOPB;
			termnew_port[i].c_cc[VTIME]  = 20;                    //VTIME:非cannoical模式读时的延时，以十分之一秒位单位
			termnew_port[i].c_cc[VMIN] = LENGHTH ;                       //VMIN:非canonical模式读到最小字符数
			tcflush(fd[i],TCIFLUSH);                         // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
			if((tcsetattr(fd[i], TCSANOW,&termnew_port[i]))!=0)   //TCSANOW:改变立即发生
			{
				perror("com set error");
				return -1;
			}
			//perror(" set done!");
	}
	  return 0;
}
void   alarm_handler() 
{
	print_result() ;  //show
	alarm(5) ;
	if(loop_mode)
	{
		test_time_out++;
		if(test_time_out==4)  //超时
		{
			int fd = 0 ;
			int ret = 0 ;
			char buf[8] = {'e', ' ', 'e', ' ', 'e', ' ', '\0'} ;
			fd = open("test_log.txt", O_RDWR | O_TRUNC | O_CREAT, 0666) ;
			ret = write(fd, buf, sizeof(buf)) ;
			close(fd) ;
			exit(0) ;
		}
	}
}

int main(int argc, char* argv[])
{
	int ret = 0;
		
	char write_txbuf[LENGHTH] = STRING;
	char read_rxbuf[LENGHTH]={0} ; 
	
	if(argc < 2)
	{
		printf("---args error---\n") ;
		return -1;
	}
	
	if( ports_cfg(argv[1], argv[2]) < 0) 
	{
		printf("初始化 %s  %s 失败\n",argv[1], argv[2]) ;
		return -1 ;
	}
	if(argc > 3)
	{
		if(strcmp(argv[3], "-d")==0)   //测试模式 
		{
			loop_mode = 1 ;
			int show_fd =0 ;
			show_fd = open("show.txt", O_RDWR) ;
			if(show_fd > 0)   //需要显示
			{
				display = 1 ;
				printf("\n--send_success-- --send_fail-- --receive_success-- --receive_fail--\n") ;
				close(show_fd) ;
			}
		}
	}
	else
			printf("\n--send_success-- --send_fail-- --receive_success-- --receive_fail--\n") ;

	
	signal(SIGALRM, alarm_handler) ;
	alarm(5) ;

	while(1)
	{
		ret = write(fd[2], write_txbuf,sizeof(write_txbuf));	
		if(ret == LENGHTH)
			date_info.send_success++ ;
		else
			date_info.send_fail++ ;
		//printf("send    %d--> %s\n", ret, write_txbuf) ;
		memset(read_rxbuf, 0, sizeof(read_rxbuf)); 
		ret = read(fd[1], read_rxbuf, sizeof(read_rxbuf));
		
		if(strcmp("123456789", read_rxbuf) == 0)
			date_info.receive_success++ ;
		else
			date_info.receive_fail++ ;
		//if(srtcmp("-d", argv[3]) == 0)
		//printf("receive %d--> %s\n", ret, read_rxbuf) ;
	    switch(loop_mode)
		{
			case 0:
				break;
			case 1:
			    if(loop_time > 1)
				{
					int fd = 0 ;
					char buf[8] = {'0', ' ', '0', ' ', '0', ' ', '\0'} ;
					
					fd = open("test_log.txt", O_RDWR | O_TRUNC | O_CREAT, 0666) ;
			        //send fail
					if(date_info.send_fail >0)       
					{
					     buf[0] = 1 ;  buf[2] = 1 ;
					}
					//receive fail
					if(date_info.receive_fail > 0) 
					{
						buf[0] = 1 ;  buf[4] = 1 ;
					}
					//成功
					ret = write(fd, buf, sizeof(buf)) ;
					 
					close(fd) ;
					goto END;
		            
				}
				break;
			default:break;
		}
		
		
	}
END:
	close(fd[1]) ;
	close(fd[2]) ;
	tcsetattr(fd[1], TCSANOW, &termold_port[1]);
	tcsetattr(fd[2], TCSANOW, &termold_port[2]);
	
	return 0 ;
}















