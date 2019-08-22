#include<stdlib.h>
#include <stdio.h>
#include <time.h>



int main()
{
    int bootdelay = 10;
    printf("Hit any key to stop autoboot: %2ds ", bootdelay);
    fflush(stdout);
    while(bootdelay > 0)
    {
        sleep(1);
        --bootdelay;
        printf("\b\b\b\b%2ds ", bootdelay);
        fflush(stdout);

    }
}
