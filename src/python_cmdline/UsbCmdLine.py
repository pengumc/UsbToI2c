#! /usr/bin/env python
# author: Michiel van der Coelen 
# "Is there someone else up there we can talk to?"

#constants: 	CAPS
#"private":		prefix _
#methods/functions:	lower_case
#classes:			CamelCase
#other:				camelCase


import usb.core
import sys
from xml.etree.ElementTree import ElementTree

class UsbAdapter:
    _USB_TYPE_VENDOR = (2<<5)
    _USB_RECIP_DEVICE = 0
    _USB_ENDPOINT_IN = (1<<7)
    _USB_ENDPOINT_OUT= (0<<7)

    def __init__(self):
        self.servoAmount = 0
        self.servoMaxPulse = 0
        self.servoMinPulse = 0
        
        self._buffer = None
        self._VID = None
        self._PID = None
        self._dev = None
    
    def load_xml(self, fileName):
        tree = ElementTree(file=fileName)
        config = tree.getroot()
        devices =  config.find("device")
        self._VID = int(devices.find("VID").text,16)
        self._PID = int(devices.find("PID").text,16)
        self.servoMaxPulse = int(devices.find("max_pulse").text)
        self.servoAmount = int(devices.find("servo_amount").text)
        self.servoMaxPulse = int(devices.find("min_pulse").text)

        pos = devices.find("startpos").findall("pos")
        self._buffer = bytearray(self.servoAmount)
        for i in range(self.servoAmount):
            self._buffer[i] = int(pos[i].text)
        
    def connect(self):
        self._dev = usb.core.find(idVendor=self._VID, idProduct=self._PID)
        if self._dev is None:
            print(
            "device not found  PID: 0x{0:X} | VID:0x{1:X}".format(
            self._PID, self._VID))
            return 1
        else:
            return 0
    
    def send_ctrl_transfer(self, endpoint, request, servo=0, pos=0, cmd=0):
        if self._dev is None:
           return 1
        if endpoint is self._USB_ENDPOINT_IN:
            try:
                data = self._dev.ctrl_transfer(
                    self._USB_TYPE_VENDOR | self._USB_RECIP_DEVICE | endpoint,
                    request,
                    servo,
                    pos,
                    self.servoAmount,
                    1000)
            except usb.core.USBError as e:
                print(e)
            else:
                 print(data)

        if endpoint is self._USB_ENDPOINT_OUT:
            zero = bytearray(12)
            for i in range(12):
                zero[i] = cmd
            try:
                cnt = self._dev.ctrl_transfer(
                    self._USB_TYPE_VENDOR | self._USB_RECIP_DEVICE | endpoint,
                    request,
                    servo,
                    pos,
#                    self._buffer,
                    zero,
                    1000)
            except usb.core.USBError as e:
                print(e)
            else:
                print("send %s bytes" % (len(self._buffer)))
                print zero[0]

#-----------------------------------------------------------------------------
x = UsbAdapter()
x.load_xml("config.xml")
x.connect()
x.send_ctrl_transfer(UsbAdapter._USB_ENDPOINT_IN,int(sys.argv[1]))
