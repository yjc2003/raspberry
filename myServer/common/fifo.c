#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include "fifo.h"

#define FIFO_DIR "/tmp/myhttpsvr"
#define BUF_SIZE 512      // must less than PIPE_SIZE that operate with atomic

#ifdef FIFO_READ

static pthread_t thread=0;
static int fifo_fd=-1;
static char szReadValue[BUF_SIZE+1]={0,};
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int createFifo(const char *name) {

	char path[255];
	sprintf(path,"%s/%s",FIFO_DIR,name);
	
	fifo_fd=-1;
		
	remove(path);
	
	if(access(FIFO_DIR,0)!=0) {  // if directory not exist, make it
		
		char cmd[300];
		sprintf(cmd,"mkdir -p %s",FIFO_DIR);
		if(system(cmd)!=0) {
			perror("mkdir -p failed");
			return -1;
		}
	}
	
	if(-1==mkfifo(path,0666)) {
		perror("mkfifo error");
		return -1;
	}
	
	fifo_fd=open(path,O_RDWR);
	if(fifo_fd==-1) {
		perror("open fifo error");
		return -1;
	}
	
	return 0;
}

void getFifoValue(char *buf,int buflen) {
    
	int len;
	
	pthread_mutex_lock(&mutex); 
	
	len=strlen(szReadValue);
	if(buflen<(len+1)) {
		fprintf(stderr,"buf size is small\n");
		buf[0]=0;
		return;
	}
	memcpy(buf,szReadValue,len);
	buf[len]=0;

	pthread_mutex_unlock(&mutex); 
}

static void putValue(char *buf,int read_len) {
	pthread_mutex_lock(&mutex); 
		
	memcpy(szReadValue,buf,read_len);
	szReadValue[read_len]=0;
		
	pthread_mutex_unlock(&mutex); 
}

static void* run(/*void* arg*/) {
	char buf[BUF_SIZE];
	int read_len;
		
	printf("fifo run start\n");
	
	if(fifo_fd<0) {
		fprintf(stderr,"fifo not created\n");	
		goto exit;
	}
	
	while(1) {
		
		//printf("waiting read from fifo...\n");
		read_len=read(fifo_fd,buf,sizeof(buf));
		if(read_len<=0) {
			if(EINTR==errno) {
				printf("read error EINTR, so that continue to read\n");
				continue;
			}
			perror("fifo read error");
			
			goto exit;
		}
		
		putValue(buf,read_len);
	}
	
exit:	
	
	printf("fifo run exit\n");
	return NULL;
}

int start_readFifo() {
	
	int id;
	if(thread!=0) {
		fprintf(stderr,"fifo read thread is running already\n");
		return -1;
	}
		
	id=pthread_create(&thread,NULL,run,NULL);
	if(id<0) {
		perror("pthread create fail");
		return -1;
	}
	
	return 0;
}

#endif  // FIFO_READ


#ifdef FIFO_WRITE

static int fifo_write_fd=-1;

int openFifo_write(const char *name) {

	char path[255];
	sprintf(path,"%s/%s",FIFO_DIR,name);

	fifo_write_fd=open(path,O_RDWR);
	if(fifo_write_fd==-1) {
		perror("open(for write) fifo error");
		return -1;
	}
	
	signal(SIGPIPE, SIG_IGN); // for EPIPE error 
	
	return 0;
}

void writeFifo(char *buf,int len) {
	
	if(fifo_write_fd==-1) {
		fprintf(stderr,"fifo(write) not opened\n");	
	}
	
	if(len>BUF_SIZE) {
		fprintf(stderr,"length of data to write must be less than atomic PIPE_SIZE(=BUF_SIZE)");
		return;
	}
	
	int more=false;
	do {
	
		int write_len=write(fifo_write_fd,buf,len);
		if(write_len<len || write_len<=0) {
			if(errno == EINTR ) {
				printf("write error EINTR, so that continue to write\n");
				more=true;
				continue;
			}else if(errno == EPIPE) {
				perror("EPIPE signal occurred when fifo write");
			}
		}
		
		more= false;
	}while(more);
}

#endif // FIFO_WRITE
