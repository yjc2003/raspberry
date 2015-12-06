
CC= g++

DEBUG= -g -O0 #-O3
COMMON_DIR=../common
INCLUDE= -I/usr/local/include -I$(COMMON_DIR)
CFLAGS= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe -DFIFO_WRITE

LDFLAGS	= -L/usr/local/lib
LDLIBS    = -lwiringPi -lpthread -lm

SOURCES=	main.c
BIN=pirTest

all: $(BIN)

$(BIN): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(CFLAGS)  $(LDFLAGS) $(LDLIBS)

#.c.o:
#	@$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ) *~ core tags $(BIN)

tags:	$(SOURCES)
	@ctags $(SOURCES)

depend:
	makedepend -Y $(SOURCES)