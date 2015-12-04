#
# Makefile:
#	wiringPi - Wiring Compatable library for the Raspberry Pi
#	https://projects.drogon.net/wiring-pi
#
#	Copyright (c) 2012 Gordon Henderson
#################################################################################
# This file is part of wiringPi:
#	Wiring Compatable library for the Raspberry Pi
#
#    wiringPi is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Lesser General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    wiringPi is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public License
#    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
#################################################################################

CC= g++

DEBUG= -g -O0 #-O3
COMMON_DIR=../common
INCLUDE= -I/usr/local/include -I$(COMMON_DIR)
CFLAGS= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe -DFIFO_WRITE

LDFLAGS	= -L/usr/local/lib
LDLIBS    = -lwiringPi -lpthread -lm

SOURCES=	main.cpp wireWrapper.cpp dht11.cpp $(COMMON_DIR)/fifo.c
BIN=sensorDaemon

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
