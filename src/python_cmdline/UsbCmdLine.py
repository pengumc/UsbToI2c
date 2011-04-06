#! /usr/bin/env python
# author: Michiel van der Coelen 
# "Is there someone else up there we can talk to?"

#constants: 	CAPS
#"private":		prefix _
#methods/functions:	lower_case
#classes:			CamelCase
#other:				camelCase


import usb.core, math, sys, time
from xml.etree.ElementTree import ElementTree

pi = math.acos(-1)
class UsbAdapter:
    #usb control constants
    _USB_TYPE_VENDOR = (2<<5)
    _USB_RECIP_DEVICE = 0
    _USB_ENDPOINT_IN = (1<<7)
    _USB_ENDPOINT_OUT= (0<<7)
    #usb custom requests as defined in requests.h
    _USBRQ_LOAD_POS_FROM_I2C =5
    _USBRQ_GET_POS = 6
    _USBRQ_SET_DATA = 3
    _USBRQ_GET_DATA = 2
    _USBRQ_RESET = 4
    #I2C commands
    _I2CCMD_RESET = 1
    _I2CCMD_LOAD_STARTPOS = 2
    _I2CCMD_SAVE_STARTPOS = 3

    def __init__(self):
        self.servoAmount = 0
        self.servoMaxPulse = 0
        self.servoMinPulse = 0
        self.received = None #buffer to stored data returned
        
        self._buffer = None #buffer to store data to be send
        self._lengths = [] #array for leg lenghts
        self._K = 1.0
        self._VID = None
        self._PID = None
        self._dev = None
        self._hxErr = 0#[0.0, 0.0, 0.0, 0.0]
        self._vxErr = [0.0, 0.0, 0.0, 0.0]
    
    def load_xml(self, fileName):
        tree = ElementTree(file=fileName)
        config = tree.getroot()
        devices =  config.find("device")
        self._VID = int(devices.find("VID").text,16)
        self._PID = int(devices.find("PID").text,16)
        self.servoMaxPulse = int(devices.find("max_pulse").text)
        self.servoAmount = int(devices.find("servo_amount").text)
        self.servoMaxPulse = int(devices.find("min_pulse").text)
        self._K = float(devices.find("K").text)

        self.received = bytearray(self.servoAmount)
        self._buffer = bytearray(self.servoAmount)
        lengths = devices.find("leglength").findall("cm")
        for i in range(len(lengths)):
            self._lengths.append(float(lengths[i].text))
        
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
        if endpoint == "in":
            try:
                self.received = self._dev.ctrl_transfer(
                    self._USB_TYPE_VENDOR | self._USB_RECIP_DEVICE |
										self._USB_ENDPOINT_IN,
                    request,
                    servo,
                    pos,
                    self.servoAmount,
                    1000)
            except usb.core.USBError as e:
                print(e)
            else:
                pass
                #print(self.received)

        if endpoint == "out":
            if cmd <> 0:
                for i in range(self.servoAmount):
                    self._buffer[i] = cmd
            try:
                cnt = self._dev.ctrl_transfer(
                    self._USB_TYPE_VENDOR | self._USB_RECIP_DEVICE | 
										self._USB_ENDPOINT_OUT,
                    request,
                    servo,
                    pos,
                    self._buffer,
                    1000)
            except usb.core.USBError as e:
                print(e)
            else:
                pass
                #print("send %s bytes" % (len(self._buffer)))
                #print(self._buffer)
                    
    def copy_received_to_buffer(self):
        for i in range(self.servoAmount):
            self._buffer[i] = self.received[i]

    def update(self, servo, amount=1):
        self.send_ctrl_transfer("in", self._USBRQ_LOAD_POS_FROM_I2C)
        self.send_ctrl_transfer("in", self._USBRQ_GET_POS)
        self.copy_received_to_buffer()
        self._buffer[servo] += amount
        self.send_ctrl_transfer("out",self._USBRQ_SET_DATA)

    def print_positions(self, servo=80):
        self.send_ctrl_transfer("in", self._USBRQ_LOAD_POS_FROM_I2C)
        self.send_ctrl_transfer("in", self._USBRQ_GET_POS)
        if servo < self.servoAmount:
            print(self.received[servo])
        else:
            print self.received

    def set_all(self, pos):
        for i in range(self.servoAmount):
            self._buffer[i] = pos
        self.send_ctrl_transfer("out", self._USBRQ_SET_DATA)

    def force_eeprom_reload(self):
        for i in range(self.servoAmount):
            self._buffer[i]= self._I2CCMD_LOAD_STARTPOS
        self.send_ctrl_transfer("out", self._USBRQ_SET_DATA)

    def angle(self, servo):
        if servo == 2: #2 5 8 11 special case
            return (72- self._buffer[servo]) * self._K - (pi/2)
        elif servo == 5:
            return (72 - self._buffer[servo]) * self._K - (pi/2)
        elif servo == 8:
            return 0
        elif servo == 11:
            return 0
        else:
            return (self._buffer[servo] - 72) * self._K

        
    def get_pos(self):        
        self.send_ctrl_transfer("in", self._USBRQ_LOAD_POS_FROM_I2C)
        self.send_ctrl_transfer("in", self._USBRQ_GET_POS)
        self.copy_received_to_buffer()

    def send_pos(self):
        self.send_ctrl_transfer("out", self._USBRQ_SET_DATA)

    def get_hx(self, leg0):
        a = self.angle(leg * 3)
        b = self.angle(leg * 3 + 1)
        c = self.angle(leg * 3 + 2)
        A = self._lengths[leg * 3]
        B = self._lengths[leg * 3 + 1]
        C = self._lengths[leg * 3 + 2]
        x = math.cos(a) * (A + math.cos(b)*B + math.cos(c+b)*C)
        return x    

    def get_vx(self, leg):
        b = self.angle(leg * 3 + 1)
        c = self.angle(leg * 3 + 2)
        B = self._lengths[leg * 3 + 1]
        C = self._lengths[leg * 3 + 2]
        x = (math.cos(b)*B + math.cos(c+b)*C)
        return x    

    def up(self, leg=0, amount = 1.0):
        if leg%2:
            B = self._lengths[leg * 3 + 1]
            C = self._lengths[leg * 3 + 2]

            x = self.get_vx(leg)
            print "---"
            print x
            print self._vxErr[leg]
            x -= self._vxErr[leg]
            print x 
            self._buffer[leg * 3 + 1] += int(amount)
            b = self.angle(leg * 3 + 1)
            gamma = -(math.acos( (x - B * math.cos(b)) / C ) - b)
#            print gamma
            self._buffer[leg*3+2] = int((gamma + pi/2) / self._K)+72
            
            #save error
            self._vxErr[leg] = self.get_vx(leg) - x

        else:
            #get current x
            x = self.get_vx(leg) - self._vxErr[leg]
            #print x
            #increase beta
            self._buffer[leg * 3 + 1] -= int(amount)
            #calculate gamma with old x
            b = self.angle(leg * 3 + 1)
            B = self._lengths[leg * 3 + 1]
            C = self._lengths[leg * 3 + 2]
            gamma = -math.acos( (x - B * math.cos(b)) / C ) - b
            #print gamma
            #update servo
            self._buffer[leg * 3 + 2] = int(72 - (gamma + pi/2) / self._K)
            #save error
            self._vxErr[leg] = self.get_vx(leg) - x 

    def clear_errors(self):
        self._vxErr = [0.0, 0.0, 0.0, 0.0]
        self._hxErr = [0.0, 0.0, 0.0, 0.0]
            
    def forward(self, leg=0, amount = 1.0):
        print "---"
        print self._hxErr
        x = self.get_hx(0) - self._hxErr
        print self.angle(0)
        print x
        print ""
        self._buffer[0] += 1
        print self.angle(0)
        print self.get_x(0)
        print ""
        a = self.angle(0)
        b = self.angle(1)
        A = self._lengths[0]
        B = self._lengths[1]
        C = self._lengths[2]
        gamma = -math.acos( (x ) / (C * math.cos(a))
                           - A / C
                           - B * math.cos(b) / C
                           ) - b
        print gamma
        self._buffer[2] = int(72-(gamma + pi/2)/self._K)
        print( 72-(gamma+pi/2)/self._K)
        print ""
        oldX = x
        x= self.get_hx(0)
        print x
        self._hxErr = x - oldX
        print self._hxErr
        print"---"

#-----------------------------------------------------------------------------
x = UsbAdapter()
x.load_xml("config.xml")
if x.connect():
    print("exiting...")
    exit(1)

#cmdline parsing
argc = len(sys.argv)
if argc == 1:
    print("invalid arguments")
    exit(2)
if sys.argv[1] == "reload":
    x.force_eeprom_reload()
elif sys.argv[1] == "update":
    if argc < 4:
        print("not enough arguments")
    else:
        x.update( int(sys.argv[2]), int(sys.argv[3]))
elif sys.argv[1] == "show":
    if argc > 2:
        x.print_positions(int(sys.argv[2]))
    else:
        x.print_positions()
elif sys.argv[1] == "seta":
    x.set_all(int(sys.argv[2]))

elif sys.argv[1] == "up":
    x.get_pos()
    for i in range(10):
        x.up(0,1)
        x.up(1,1)
        x.send_pos()
        time.sleep(0.3)

elif sys.argv[1] == "down":
    x.get_pos()
    for i in range(10):
        x.up(0,-1)
        x.up(1,-1)
        x.send_pos()
        time.sleep(0.3)
elif sys.argv[1] == "forward":
    x.clear_errors()
    x.get_pos()
    for i in range(5):
        x.forward(0,1)
        #x.forward(1,1)
        x.send_pos()
        time.sleep(0.3)
elif sys.argv[1] == "backward":
    x.clear_errors()
    x.get_pos()
    for i in range(5):
        x.forward(0,-1)
        #x.forward(1,-1)
        x.send_pos()
        time.sleep(0.3)
