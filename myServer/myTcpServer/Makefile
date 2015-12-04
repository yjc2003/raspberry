
CC=gcc
MONGOOSE_DIR=../mongoose-6.0
CFLAGS_EXTRA = -I$(MONGOOSE_DIR)
CFLAGS = -W -Wall -pthread -ldl -lm $(CFLAGS_EXTRA)
BIN = myTcpServer
SOURCES = myTcpServer.c $(MONGOOSE_DIR)/mongoose.c

all: $(BIN)

$(BIN): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(CFLAGS)

clean:
	rm -rf *.gc* *.dSYM *.obj *.o a.out $(BIN)


