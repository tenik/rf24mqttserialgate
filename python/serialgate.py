#!/usr/bin/python
# -*- coding: utf-8
__author__ = 'admin'

import serial
import time
import sys
##import MySQLdb
import socket
import mosquitto
import logging


def processCommand(inParams):
    if inParams[0] == '0': return
    if inParams[1] == 'CON':  # process CONnection
        devTopic = '/dev/'+str(inParams[0])+''
        if client.publish(devTopic, str('connected'))[0]: logging.debug(u'"%s" not published!'%devTopic)
        else:
            port.write(str(inParams[0])+' CONACK\n')
            #logging.debug(u'"%s"'%(str(inParams[0])+' CONACK'))
            #sys.stdout.write(str(inParams[0])+' CONACK\n')
    elif inParams[1] == 'PING':  # process PING
        port.write(str(inParams[0])+' PINGRESP\n')
        #sys.stdout.write(str(inParams[0])+' PINGRESP\n')

    elif inParams[1] == 'SUB':  # process SUBscribe
        nodeNumber = str(inParams[0])
        devTopic = str(inParams[2])
        if client.subscribe(devTopic)[0]: logging.debug(u'"%s" not subscribed!'%devTopic)
        else:  # add subs to list
            subs = subsList.get(devTopic)
            if subs is None:  # no subs for this topic, create topic with this node
                subsList[devTopic] = [nodeNumber]
                logging.debug(u'Created record in subs list for "%s" !'%devTopic)
            else:
                if nodeNumber not in subs:
                    subs.append(nodeNumber)  # add node if node not in subs list
                    logging.debug(u'Added node to subs list for "%s" !'%devTopic)

    elif inParams[1] == 'PUB':  # process PUBlish
        devTopic = str(inParams[2])
        if client.publish(devTopic, str(inParams[3]))[0]: logging.debug(u'"%s" not published!'%devTopic)
    else:
        pass


def on_log( obj, level, string):
    logging.debug(u'LOG>>%s!' % string)

def on_publish(obj, mid):
    logging.debug( u'Message %s published!'%mid)

def on_message(obj, msg):
    logging.debug( u'Message [%s][%s] received!'%(msg.topic,msg.payload))
    # find all recepients and send message
    for nodeNumber in subsList.get(msg.topic):
        value = nodeNumber+' PUB '+msg.topic+' '+msg.payload+'\n'
        port.write(value)
        logging.debug( u'Message [%s]sensed!'%(value))
        #sys.stdout.write(nodeNumber+' PUB '+msg.topic+' '+msg.payload+'\n')

def on_connect( obj, rc):
    if rc == 0:
        logging.debug( u'MQTT brocker connected!')
    else:
        logging.debug( u'MQTT brocker NOT connected(rc=%d)!'%rc)

def prepareSerialInputBuffer():
    line = []
    while True:
        if port.inWaiting():
             c = port.read()
             line.append(c)
             if c == '\n':
                 line = []
                 break
	else:
	     break

def readSerialInputBuffer():
    global serialTempString
    global serialInputString
    if port.inWaiting():
        while True:
            if port.inWaiting():
                c = port.read()
                serialTempString.append(c)
                if c == '\n':
                    serialInputString = serialTempString[:]
                    serialTempString = []
                    return True
            else: return False


#logging.basicConfig(level = logging.DEBUG)
#logging.basicConfig(format = u'%(filename)s[LINE:%(lineno)d]# %(levelname)-8s [%(asctime)s]  %(message)s', level = logging.DEBUG, filename = u'/home/pi/serialgate.log')
logging.basicConfig(format = u'%(filename)s[LINE:%(lineno)d]# %(levelname)-8s [%(asctime)s]  %(message)s', level = logging.DEBUG, filename = u'serialgate.log')

logging.info( u'Starting gate script...' )

port = serial.Serial("/dev/ttyAMA0", baudrate=9600, timeout=0.1)
#port = sys.stdin

# Connect to MQTT broker.
client = mosquitto.Mosquitto("serialgate")
client.on_log = on_log
client.on_publish = on_publish
client.on_message = on_message
client.on_connect = on_connect

if client.connect('127.0.0.1'):
    logging.debug(u'Error connecting  MQTT brocker!')

subsList = {'':[]} #empry subscriptions list

prepareSerialInputBuffer()
logging.debug(u"Serial input buffer cleared")

serialTempString = []
serialInputString = []

while True:
    if readSerialInputBuffer():
        logging.debug(u'Received: '+str(''.join(serialInputString)))
        params = ''.join(serialInputString).strip().split()
        if len(params)>1: processCommand(params)


    client.loop()
   # time.sleep(0.1)
