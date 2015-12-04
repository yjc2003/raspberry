
CC=gcc

COMMON_DIR=../common
MONGOOSE_DIR=../mongoose-6.0
DEBUG=-g -DMG_ENABLE_DEBUG
CFLAGS_EXTRA = -I$(MONGOOSE_DIR) -I$(COMMON_DIR) -DMG_ENABLE_THREADS -DFIFO_READ #$(DEBUG)
CFLAGS = -W -Wall -pthread -ldl -lm $(CFLAGS_EXTRA)

BIN = myHttpServer
SOURCES = myHttpServer.c $(MONGOOSE_DIR)/mongoose.c $(COMMON_DIR)/fifo.c
#SOURCES = myHttpServer.c sqlite3.c db_plugin_sqlite.c $(MONGOOSE_DIR)/mongoose.c $(COMMON_DIR)/fifo.c

all: $(BIN)

$(BIN): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(CFLAGS)

clean:
	rm -rf *.gc* *.dSYM *.obj *.o a.out $(BIN)

