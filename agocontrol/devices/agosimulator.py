#! /usr/bin/env python

import random
import sys
import syslog
import socket
import threading
import time

import agoclient

client = agoclient.AgoConnection("simulator")

def messageHandler(internalid, content):
	if "command" in content:
		if content["command"] == "on":
			print "switching on: " + internalid
			client.emit_event(internalid, "event.device.statechanged", "255", "")
		if content["command"] == "off":
			print "switching off: " + internalid
			client.emit_event(internalid, "event.device.statechanged", "0", "")
		if content["command"] == "push":
			print "push button: " + internalid
		if content['command'] == 'setlevel':
			if 'level' in content:
				print "device level changed", content["level"]
				client.emit_event(internalid, "event.device.statechanged", content["level"], "")

client.add_handler(messageHandler)

client.add_device("123", "dimmer")
client.add_device("124", "switch")
client.add_device("125", "binarysensor")
client.add_device("126", "multilevelsensor")
client.add_device("127", "pushbutton")

class testEvent(threading.Thread):
    def __init__(self,):
        threading.Thread.__init__(self)    
    def run(self):
    	level = 0
	counter = 0
        while (True):
		counter = counter + 1
		if counter > 3:
			counter = 0
			temp = random.randint(50,300) / 10
			client.emit_event("126", "event.environment.temperaturechanged", temp, "degC");
			client.emit_event("126", "event.environment.humiditychanged", random.randint(20, 75), "percent");
		client.emit_event("125", "event.security.sensortriggered", level, "")
		if (level == 0):
			level = 255
		else:
			level = 0
		time.sleep (5)
      
background = testEvent()
background.setDaemon(True)
background.start()

syslog.syslog(syslog.LOG_NOTICE, "agosimulator.py startup")
client.run()

