/*

 *
 * Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include "wireWrapper.h"
#include "dht11.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "fifo.h"

#define FIFO_FILENAME "tmhu"

struct TMHU {
	double temperature;
	double humidity;

	TMHU() {
		temperature=humidity=0;
	}
	
	TMHU(double temperature,double humidity) {
		this->temperature=temperature;
		this->humidity=humidity;
	}
};

static TMHU g_value;

void setHuTm(DHT11 *dht) {
	g_value.temperature= dht->getDHTTemperatureC();
	g_value.humidity=dht->getDHTHumidity();
	
	printf("temperature:%f, humidity:%f\n",g_value.temperature,g_value.humidity);
}


void writeToFifo() {
    
	char buf[100];
	sprintf(buf,"%f %f",g_value.temperature,g_value.humidity);
	writeFifo(buf,strlen(buf));
}

/*
char* getMessage() {

	time_t current_time;
	struct tm *st;

	time(&current_time);
    st = localtime( &current_time);
	
	char strDate[30];
	sprintf(strDate,"%04d%02d%02d %02d%02d%02d",st->tm_year+1900,st->tm_mon+1,st->tm_mday,
	st->tm_hour,st->tm_min,st->tm_sec);
	
	char* buf= new char[100];
	sprintf(buf,"[%s] temperature=%d , humidity=%d\n",strDate,g_value.tm,g_value.hu);
	return buf;
}


bool lockFile(int fd) {

	 struct flock filelock;
	 filelock.l_type   = F_WRLCK;
     filelock.l_whence = SEEK_SET;
     filelock.l_start  = 0;
     filelock.l_len    = 0; //strlen(msg);
	 
	 if (fcntl( fd, F_SETLKW, &filelock)!=-1) {
		return true;
	 }else {
		printf("error fnctl\n");
		return false;
	 }
	
	 return true;
}


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


char* readFromFile(char* filepath) {

	int fd= open(filepath,O_RDONLY);
	
	if(fd>0) {
		if(!lockFile(fd)) {
			return NULL;
		}
	
		struct stat st;
		stat(filepath,&st);
		char* buf = new char[st.size+1];
		
		int tot=0;
		do {
			int len= read(fd,buf,st.size);
			if(len<=0) {
				break;
			}
			tot+=len;
		}while(tot<st.size);
	
		fclose(fd);
		
		if(tot==st.size) {
			return buf;
		}
		delete [] buf;
	}
	
	return NULL;
}


void writeToFile(int fd) {
	char* msg = getMessage();
	if(lockFile(fd)) {
		write(fd,msg,strlen(msg)); 
	}
	delete [] msg;
}

void writeToLogFile(char*filepath) {
	int fd= open(filepath,O_RDWR |O_APPEND| O_CREAT, 0644);
	if(fd>0) {
		writeToFile(fd)
		close(fd);
	}
}

void writeToStdout() {
	writeToFile(stdout);
}


bool checkFile(char* filepath) {

	int fd= open(filepath,O_RDWR |O_TRUNC| O_CREAT, 0644); //O_APPEND
	if(fd<0) {
		return false;
	}
	
	close(fd);
}
*/



int main(int argc, char *argv[])
{
	const char *filename=NULL;
	int intervalTime=1000; //millsec
	int gpioPin=0;
	int c; 
	
	setvbuf (stdout,NULL,_IONBF,0);
	
	while( (c = getopt(argc, argv, "ht:p:o:")) != -1) {
		
		switch(c) {
			case 'h':
				printf("Usage: sensorfetcher [-p pipe_filename ] [-t time_interval(millsec)] [-o gpioNum]\n");
				return 1;
			case 'p':
				filename=optarg;
                 break;
			case 't':
				intervalTime= atoi(optarg);
				if(intervalTime<=0) {
					intervalTime=1000;
				}
				break;
			case 'o':
				gpioPin=atoi(optarg);
				if(gpioPin<0 || gpioPin>8) {
					fprintf(stderr,"gpioPin invalid parameter, so adjust it to default\n");
					gpioPin=0;
				}
             case '?':
				 if(optopt == 'p') {
				     printf("option -f requires filename\n");
				 }else {
					 printf("Unknown flag : %c", optopt);
				 }
                 return 1;
	    }
     }
	 
	 if(filename==NULL) {
		filename=FIFO_FILENAME;
	 }
	 
	 printf("pipe filename: %s\n",filename);
	 printf("intervalTime set to %d\n",intervalTime);
	 printf("goioPin set to %d\n",gpioPin);
	 	
    if(wiringPiSetup() == -1) {
		fprintf(stderr,"wiringPiSetup fail\n");
		return 1;
	}
	
	if(openFifo_write(filename)!=0) {
		fprintf(stderr,"openFifo_write fail\n");
		return 1;
	}
	
	printf("start sensing Humidiy & Temperature\n");
	
    DHT11 dht = DHT11(gpioPin);
		    
    while(true){
        
        dht.requestDHT11();
        if(dht.responseDHT11() == PASS){
        	if(dht.calculateDHTValue() == true){
											
				setHuTm(&dht);
				writeToFifo();
				// <TODO> 버퍼를 비우는 프로세스가 없어서 파이프가 일정크기만크 찼을경우
				//  (count 가 일정회수 되면 파이프 크기체크해서 , 넘으면 truncate 시키자)
        	}
        }

		delay(intervalTime);
    }  
    
    return 0;
}
