#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include<time.h>  //getSystemTimer
#include<sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>

struct DATE{
    unsigned long receive_success;
    unsigned long receive_fail;
};
static struct DATE date_info = {0, 0} ;

static int  loop_time = 0 ;
static int  time_out_flag = 0 ;  //测试模式
static int  time_out = 0 ;
static int display = 0 ;

void   alarm_handler()  ;


void getSystemTimer(void)
{
    struct timeb timebuffer;  //get ms
    static long  oldsecond ;  long msecond = 0 ;

    static  char flag=0;
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

    ftime(&timebuffer);  
    msecond = (tv.tv_sec-oldsecond)*1000 + timebuffer.millitm ;
    oldsecond = tv.tv_sec ;

    if(time_out_flag ==1 && display == 0) ////只有工厂测试模式指定下 才不需要显示
        ;
    else
        printf("        % 6d              % 6d\n",date_info.receive_success, date_info.receive_fail) ;

    loop_time++;

}

void   alarm_handler() 
{
    getSystemTimer() ;
    alarm(5) ;
    if(time_out_flag = 1)
    {
        time_out++;
        if(time_out==4)
        {
            int fd = 0 ;
            int ret = 0 ;
            char buf[6] = {'e', ' ', 'e', ' ', '\0'} ;
            fd = open("d_can_log.txt", O_RDWR | O_TRUNC | O_CREAT, 0666) ;
            ret = write(fd, buf, sizeof(buf)) ;
            close(fd) ;
            system("killall can_aotu") ;
            exit(0) ; 
        }
    }
}
void cfg_can(void)
{
    system("canconfig can0 restart-ms 1000 bitrate 100000 ctrlmode triple-sampling on") ;
    system("canconfig can0 start");
    system("canconfig can1 restart-ms 1000 bitrate 100000 ctrlmode triple-sampling on") ;
    system("canconfig can1 start");
    system("sync") ;
}
int main(int argc, char *argv[])
{
    int s, nbytes, ncount;
    char *array[2] = {"-r", "-s"};
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    struct can_filter rfilter[1];


    /* handle (optional) flags first */
    if(argc < 3) {
        fprintf(stderr, "Usage:  %s <-r> <can interface name> for receiving\nor <-s> <can interface name> for sending\n", argv[0]);
        exit(1);
    }
    /* create socket */
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        perror("Create socket failed");
        exit(-1);
    }

    /* set up can interface */
    strcpy(ifr.ifr_name, argv[2]);
    /////////printf("can port is %s\n",ifr.ifr_name);
    /* assign can device */
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    /* bind can device */
    if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind can device failed\n");
        close(s);
        exit(-2);
    }

    /* configure receiving */
    if(!strcmp(argv[1],array[0]))
    {
        signal(SIGALRM, alarm_handler) ;
        alarm(5) ;

        /* set filter for only receiving packet with can id 0x1F */
        rfilter[0].can_id = 0x1F;
        rfilter[0].can_mask = CAN_SFF_MASK;
        if(setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0)
        {
            perror("set receiving filter error\n");
            close(s);
            exit(-3);
        }
        if(argc > 3)
        {
            if(!strcmp(argv[3], "-d"))
            {
                time_out_flag = 1 ;
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
        /* keep reading */
        while(1){
            if(time_out_flag == 1)
            {
                if(loop_time > 2)
                {
                    int fd = 0 ;
                    int ret = 0 ;
                    char buf[6] = {'0', ' ', '0', ' ', '\0'} ;

                    fd = open("d_can_log.txt", O_RDWR | O_TRUNC | O_CREAT, 0666) ;
                    if(date_info.receive_fail >0)
                    {
                        buf[0] = 1 ;  buf[2] = 1 ;
                    }
                    ret = write(fd, buf, sizeof(buf)) ;
                    close(fd) ;
                    goto finish ;
                }
            }
            nbytes = read(s, &frame, sizeof(frame));
            if(nbytes > 0)
            {
                for(int i=0; i<frame.can_dlc; i++)
                {
                    if(frame.data[i]==i)
                        ncount++;
                }
                if(ncount == 8)
                    date_info.receive_success++;
                else
                    date_info.receive_fail++;

                ncount=0;
                for(int n=0;n<8;n++)  //清空
                    frame.data[n] = 0 ;
            }
        }
    }
    /* configure sending */
    else if(!strcmp(argv[1],array[1]))
    {
        int send_times = 0 ;
        if(argc ==4)
        {
            if(!strcmp(argv[3], "-t"))
                send_times = 100 ;
            else
                send_times = atoi(argv[3]) ;
        }
        else
            send_times = 1  ;

        frame.can_id = 0x1F;
        frame.can_dlc = 8;
        for (int i=0; i<8; i++)
            frame.data[i] = i ;

        while(send_times)
        {
            /* Sending data */
            if(write(s, &frame, sizeof(frame)) < 0)
                perror("Send failed");
            if(!strcmp(argv[3], "-t"))
                send_times++;
            send_times--;

            usleep(1000*50) ;
        }
    }
    /* wrong parameter input situation */
    else
    {
        printf("wrong parameter input\n");
    }
finish:
    close(s);
    return 0;
}
