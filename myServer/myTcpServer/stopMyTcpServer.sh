#!/bin/sh
ps -elf | grep myTcpServer | awk '{print $4}' | xargs kill -9

