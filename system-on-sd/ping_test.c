#include<stdlib.h>
#include <stdio.h>
#include <time.h>



int main(int argc, char* argv[])
{
    int ret = 0 ;

    while(1)
    {
        ret = system(argv[1]) ;
        if(ret == 0 )
            printf("success-->") ;
        else
            printf("fail-->") ;

        printf("ret = %d\n", ret) ;
        sleep(5) ;
    }

}

