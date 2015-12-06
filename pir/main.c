#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>

#define PIR 7      //BCM #4
//#define LED 11       //BCM_GPIO 7
//HC-SR501 sensor

int main(void)
{
    if(wiringPiSetup() == -1)
    return 1;
    
    pinMode(PIR,INPUT);
    //pinMode(LED,OUTPUT);
    //  점퍼에 가까운쪽이 그라운드
    for(;;)
    {
           if(digitalRead(PIR) == 1)
           {
			   printf("detected!!!\n");
              //digitalWrite(LED,1);
           }else {
			   printf("\n");
		   }
		   
		   //usleep(2000000); // 4 sec
		   		   
           
           //else digitalWrite(LED,0);
    }
    return 0;
}