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

#define  DATE_STRING    "123456789abcdefgh"
#define  LENGHTH        18
#define  PTHREAD_PORK   0         //选择进程或者线程

static  char*   port_6[7] = {"xxx", "/dev/ttyO1","/dev/ttyO2","/dev/ttyO3","/dev/ttyO4","/dev/ttyS1","/dev/ttyS2",} ;
								
static char*   port_10[17] = {"xxx","/dev/ttymxc1","/dev/ttymxc2",
									"/dev/ttyS3","/dev/ttyS4",
									"/dev/ttyS5","/dev/ttyS6",
									"/dev/ttyS7","/dev/ttyS8",
									"/dev/ttyS9","/dev/ttyS10",
									"/dev/ttyS11","/dev/ttyS12",
									"/dev/ttyS13","/dev/ttyS14",
									"/dev/ttyS15","/dev/ttyS16"};

static  struct termios termold[17],termnew[17];

void*  read_thread(void* arg) ;    
void*  write_thread(void* arg) ;

volatile int    start_port = 1 ;
volatile int    total_ports = 0 ;
volatile static 	int retlen[17]={0} ;
volatile static 	int txlen[17]={0};
volatile static  long unsigned int rxtotal[17]={0};
volatile static  long unsigned int txtotal[17]={0};
volatile static  long unsigned int faillen[17]={0};
volatile static  int fd[17]={0};


#define  FORK_MODE
//#define  THREAD_MODE

void   alarm_post() ; 

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
	if(start_port!=1)
		return ;
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
	printf("\n*************************    %02d:%02d:%02d    **************************\n",(timeoffset/60)/60,(timeoffset/60)%60,timeoffset%60) ;  //48.8*2
	//printf("\n************************ %d:%d:%d  %1.1fKb/S*************************\n",(timeoffset/60)/60,(timeoffset/60)%60,timeoffset%60,98000.0/msecond) ;  //48.8*2
}
//uart configration
void uart_config(char uart1,char uart2)
{
	/*
	 * setting
	 */
	char i =0 ;
	for(i=uart1;i<=uart2;i++){
		if(tcgetattr(fd[i], &termold[i]) != 0)
		{
			perror("SetupSerial 1");
			return ;
		}
		bzero(&termnew[i], sizeof(termnew[i]));
		termnew[i].c_iflag &= ~(ICRNL|IGNCR) ;
		termnew[i].c_cflag |= CLOCAL | CREAD;   //CLOCAL:忽略modem控制线  CREAD：打开接受者
		termnew[i].c_cflag &= ~CSIZE; 
		termnew[i].c_cflag |= CS8;
		termnew[i].c_cflag &= ~PARENB;
		cfsetispeed(&termnew[i], B115200);
		cfsetospeed(&termnew[i], B115200);
		termnew[i].c_cflag &=  ~CSTOPB;
		termnew[i].c_cc[VTIME]  = 20;                    //VTIME:非cannoical模式读时的延时，以十分之一秒位单位
		termnew[i].c_cc[VMIN] = LENGHTH ;                       //VMIN:非canonical模式读到最小字符数
		tcflush(fd[i],TCIFLUSH);                         // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
		if((tcsetattr(fd[i],TCSANOW,&termnew[i]))!=0)   //TCSANOW:改变立即发生
		{
			perror("com set error");
			return ;
		}
		perror(" set done!");
	}
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
	for(i=start_port;i<=total_ports;i++){
	  fail = faillen[i];  
	  all = (rxtotal[i] + faillen[i]) ; //pack
	  if(faillen[i]>=all){
		  loss_pack = 1.0 ;
	  }else{
		  loss_pack = fail/all ;
	  }
	  //printf("\n==uartO%d-Tx: %ld    Rx %ld    success:%ld    fail:%ld    %1.1f %==\n",i,txtotal[i],all,rxtotal[i],faillen[i],(1.0-loss_pack)*100) ;
	 if(i<10)
		printf("\n==uartO%d-Tx:  %-7ld  Rx %-7ld  success:%-7ld  fail:%-7ld ==\n",i,txtotal[i],all,rxtotal[i],faillen[i]) ;
     else
		printf("\n==uartO%d-Tx: %-7ld  Rx %-7ld  success:%-7ld  fail:%-7ld ==\n",i,txtotal[i],all,rxtotal[i],faillen[i]) ;
	}
	//printf("*******************************************************************\n");
	  
}
void   alarm_post() 
{
	print_result() ;  //show
	alarm(5) ;
}
void port_init()
{
#ifdef   FORK_MODE
	int i = 0 ;
	pid_t pid = 0 ;
	
	for(i=1;i<=total_ports;i+=2)
	{
        pid = fork() ;
        if(pid==0)	
		{
            fd[i]=open(port_10[i],O_RDWR ) ;
            fd[i+1]=open(port_10[i+1],O_RDWR ) ;
			
			if((fd[i]==-1)||(fd[i+1]==-1)){
			    printf("\n+++++打开串口 %d %d 失败+++++\n", i, i+1) ;
				exit(-1) ;
			}
			else{
				printf("\n+++++打开串口 %d %d 成功+++++\n", i, i+1) ;
				start_port = i ;
				total_ports = i +1 ;
				uart_config(start_port,total_ports) ;
				pthread_t  fpthread_read ;
				pthread_t  fpthread_write ; 
				pthread_create(&fpthread_read,  NULL,  read_thread,  (void*)i);
				pthread_create(&fpthread_write,  NULL, write_thread,  (void*)(i+1));
				signal(SIGALRM, alarm_post) ;
				alarm(5) ;
				while(1){
						sleep(100000) ; 
				}
			}

		}	
	}
	
	while(1){
		sleep(100000) ; 
	}
#endif
}

void check_for_port()
{
    int i = 0, fd = 0;
	for(i=7;i<=16;i++){
		fd = open(port_10[i],O_RDWR) ;
		if(fd!=-1){
			total_ports = 16 ;
			close(fd) ;
			return ;
		}
		
	}
	total_ports = 6 ;
}
int test_two_port(char* port1, char* port2)
{
		static int ports_get[3] = {0} ;
		pthread_t  fpthread_read ;
		pthread_t  fpthread_write ; 
		
		ports_get[1] = atoi(port1) ;
		ports_get[2] = atoi(port2) ;
		fd[ports_get[1]] = open(port_10[ports_get[1]],O_RDWR) ;
		fd[ports_get[2]] = open(port_10[ports_get[2]],O_RDWR) ;
		if(fd[ports_get[1]]<0 || fd[ports_get[2]]<0)
		{
			if(fd[ports_get[1]]<0){
				printf("\n======串口 %d 打开失败======\n",ports_get[1]) ;
			}
			else{
				printf("\n======串口 %d 打开失败======\n",ports_get[2]) ;
			}
			exit(-1) ;
		}
		printf("\n======串口 %d %d 打开成功======\n",ports_get[1], ports_get[2]) ;
		start_port = ports_get[1] ;
		total_ports = ports_get[2] ;
		uart_config(ports_get[1],ports_get[2]) ;
	
		pthread_create(&fpthread_read,  NULL,  read_thread,  (void*)ports_get[1]);
		pthread_create(&fpthread_write,  NULL, write_thread,  (void*)ports_get[2]);
		return 0 ;

}
void tx_rx_loop_232(void)
{
	
}
int main(int argc,char* argv[])
{
	char i = 0 ;
    
	if(argc==3){
		if(test_two_port(argv[1], argv[2])==0){
			signal(SIGALRM, alarm_post) ;
			alarm(5) ;
			while(1)
			{
				sleep(1000000) ;
			}
		}else{
			printf("\n======测试失败======\n") ;
			return-1;
		}
	}
	
	check_for_port() ;
	printf("\n======检测到 %d 路串口======\n",total_ports) ;
	port_init() ;
	
    //close fd
	for(i=start_port;i<=total_ports;i++){
		tcsetattr(fd[i],TCSANOW,&termold[i]);
	    close(fd[i]);
	}
	return 0;
ERROR:
	printf("\n---所有端口打开失败 请检查线路---\n");
	printf("\n---所有端口打开失败 请检查线路---\n");
	return 0 ;
}

void*  read_thread(void* arg)
{
    char write_txbuf[LENGHTH] = DATE_STRING;
	char read_rxbuf[LENGHTH]={0} ; 
	char checkbuf[LENGHTH] = DATE_STRING;;
	//printf("线程开始进入  %d\n",paramer_in) ;
	while(1){
	//clear rxdate buf  ===read===
	memset(read_rxbuf,0,sizeof(read_rxbuf));
	retlen[(int)arg]= read(fd[(int)arg],read_rxbuf,sizeof(read_rxbuf));
	if(strcmp(checkbuf, read_rxbuf)==0){
		rxtotal[(int)arg]+=retlen[(int)arg];
	}else{
		faillen[(int)arg]+=retlen[(int)arg] ;	
		tcflush(fd[(int)arg],TCIFLUSH);       //如果接收失败 刷新缓冲 继续接收  		
	}
	//===write===
	txlen[(int)arg]= write(fd[(int)arg],write_txbuf,sizeof(write_txbuf));	
	txtotal[(int)arg]+=txlen[(int)arg] ;
	
	}
}
void*  write_thread(void* arg)
{
    char write_txbuf[LENGHTH] = DATE_STRING;
	char read_rxbuf[LENGHTH]={0} ; 
	char checkbuf[LENGHTH] = DATE_STRING;;
	//printf("线程开始进入  %d\n",paramer_in) ;
	while(1){
		txlen[(int)arg] = write(fd[(int)arg],write_txbuf,sizeof(write_txbuf)) ;
		
		memset(read_rxbuf,0,sizeof(read_rxbuf));        //clear rxdate buf 
		retlen[(int)arg] = read(fd[(int)arg],read_rxbuf,sizeof(read_rxbuf)) ;  //send after receive
		if(strcmp(checkbuf, read_rxbuf)==0){
			rxtotal[(int)arg]+=retlen[(int)arg];
			//printf("\n===rev:%s    send:%s    len:%d===\n",read_rxbuf, write_txbuf, retlen[paramer_in]) ;
		}else{
			faillen[(int)arg]+=retlen[(int)arg] ;	
			tcflush(fd[(int)arg],TCIFLUSH);       //如果接收失败 刷新缓冲 继续接收  			
		}
	    //因为read会有阻塞 所以等接收到后再加  避免发送回比接收多
		txtotal[(int)arg]+=txlen[(int)arg] ;
		
	}
}
