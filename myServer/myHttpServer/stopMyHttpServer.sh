#!/bin/sh
ps -elf | grep myHttpServer | awk '{print $4}' | xargs kill -9

