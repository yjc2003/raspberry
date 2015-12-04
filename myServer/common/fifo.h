#ifndef FIFO_H_INCLUDED_
#define FIFO_H_INCLUDED_

#ifdef FIFO_READ
	int createFifo(const char *name);
	void getFifoValue(char *buf,int buflen);
	int start_readFifo();
#endif


#ifdef FIFO_WRITE
	int openFifo_write(const char *name);
	void writeFifo(char *buf,int len);
#endif

#endif