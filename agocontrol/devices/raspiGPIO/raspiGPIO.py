#! /usr/bin/env python 

#
# ago raspberry pi GPIO device
#
# CONFDIR/conf.d/raspiGPIO.ini
#
#[raspiGPIO]
#inputs=18,23
#outputs=22,24,25
#

import agoclient
import threading
import time
import RPi.GPIO as GPIO
import syslog

client = agoclient.AgoConnection("raspiGPIO")

readInputs = agoclient.get_config_option("raspiGPIO", "inputs", "")
readOutputs = agoclient.get_config_option("raspiGPIO", "outputs", "")
reverse = agoclient.get_config_option("raspiGPIO", "reverse", "0")

GPIO.setmode(GPIO.BCM)

try:
	inputs = map(int, readInputs.split(','))
except:
	syslog.syslog(syslog.LOG_ERR, 'no valid inputs')
	inputs = None
else:
	for pin in inputs:
		#print pin
		GPIO.setup(pin, GPIO.IN)
		client.add_device(pin, "binarysensor")

try:
	outputs = map(int, readOutputs.split(','))
except:
	syslog.syslog(syslog.LOG_ERR, 'no valid outputs')
	print 'no valid outputs'
else:
	for pin in outputs:
		#print pin
		GPIO.setup(pin, GPIO.OUT)
		client.add_device(pin, "switch")

def messageHandler(internalid, content):
	if "command" in content:
		if content["command"] == "on":
			#print "switching on: ", internalid
			if reverse == "1":
				GPIO.output(internalid, False)
			else:
				GPIO.output(internalid, True)
			client.emit_event(internalid, "event.device.statechanged", "255", "")
		if content["command"] == "off":
			#print "switching off: ", internalid
			if reverse=="1":
				GPIO.output(internalid, True)
			else:
				GPIO.output(internalid, False)
			client.emit_event(internalid, "event.device.statechanged", "0", "")

client.add_handler(messageHandler)

class readGPIO(threading.Thread):
    def __init__(self,):
        threading.Thread.__init__(self)    
    def run(self):
    	prev_input = {}
        input = {}
        for pin in inputs:
            prev_input[pin] = 0
            input[pin] = 0
        print input
        while (True):
			for pin in inputs:
				input[pin] = GPIO.input(pin)
				if (not prev_input[pin]) and input[pin]:
					#print 'sensor:', pin, '255'
					client.emit_event(pin, "event.security.sensortriggered", 255, "")
				if prev_input[pin] and (not input[pin]):
					#print 'sensor:', pin, '0'
					client.emit_event(pin, "event.security.sensortriggered", 0, "")
				prev_input[pin] = input[pin]
			time.sleep(0.002)

if inputs:      
	background = readGPIO()
	background.setDaemon(True)
	background.start()

client.run()
