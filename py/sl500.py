#-------------------------------------------------------------------------------
# Name:        RFID Reader/Writer API for the SL500 RFID USB module
#
# Authors:     Arnan (Roger) Siptiakiat
#              Attaphan Chan-in
#
# Created:     04/10/2013
# Copyright:   (c) 2013Learning Inventions Lab, Chiang Mai University
# Licence:     GPL v.3
#-------------------------------------------------------------------------------

import serial
import time
import sys
import operator

## create an command array template



class RFID_Reader():
    def __init__(self):

        self.debugOn = True
        self.version = "1.0.1"

        self.data_Cerate = 0
        self.block_return = 0
        self.result_read_string = 0
        self.length_CMD = 0
        self.ser = serial.Serial()
        self.ser.setBaudrate(19200)
        self.ser.port = 6      # comport start port0 : self.ser.port = 7 is comport 8
        self.ser.timeout = 0.2


    # Connects to the RFID using the given portName or number
    # - On Windows this sould be the COM port number -1.
    #     Ex. COM1 -> portName=0, COM2 -> portName=1 ...
    # - On Linux (or the Raspberry Pi), the port name will be
    #   /dev/ttyUSBx => x = port number
    #     Ex. /dev/ttyUSB0

    def connect(self, portName):
        self.ser.port = portName
        self.ser.open()
        time.sleep(0.001)   # this is required on the Raspberry PI
                            # the RPi seems to need time to open the port before
                            # using it

    def debug(self, message):
        if self.debugOn :
            print message


    # read one byte and add to the receive checksum
    def readByte(self):
        try:
            inByte = self.ser.read()
            return(ord(inByte))
        except:
            self.debug("Serial read error, timeout")
##            sys.exit()
            raise


    # Wait for the reply header 0x55,0xFF from the GoGo
    # returns 0 if timeout (1 sec)
    # returns 1 if header is found

    def waitForHeader(self):

        timeOutCounter = 0

        while (timeOutCounter < 10):
            while (self.ser.inWaiting > 1):
                if (self.readByte()==0xAA):
                    if (self.readByte() == 0xBB):
                        return(1)

            time.sleep(0.001)
            timeOutCounter += 1

        return(0)


    def createString(self, array):
        outStr = ""
        for item in array:
            outStr += item

        return(outStr)

    def writeCommand(self, cmd):

    # ==================================================
    # Write command
    # ==================================================

        self.ser.write(self.createString(cmd))



    # ==================================================
    # Read reply
    # ==================================================

    def readReply(self):


        if self.waitForHeader() == 0:
            self.debug("Reply not found")
        else:
            Length = self.readByte()
            Length += (self.readByte() << 8)

            Deveice_ID = self.readByte()
            Deveice_ID += (self.readByte() << 8)

            Command_code =  self.readByte()
            Command_code += (self.readByte() << 8)

            Status =  self.readByte()

            if Status != 0:
                self.data_Cerate = 0

            else:
                if Command_code == 0x0104:      #0x0401 Command_code of get model
                    self.data_Cerate = 11
                elif Command_code == 0x0201:    #0x0102 Command_code of request
                    self.data_Cerate = 2
                elif Command_code == 0x0208:    #0x802 Command_code of read
                    self.data_Cerate = 16
                elif Command_code == 0x0202:    #0x202 Command_code of anticoll
                    self.data_Cerate = 4
                elif Command_code == 0x0203:    #0x302 Command_code of select
                    self.data_Cerate = 1
                else:
                    self.data_Cerate = 0


            data = []
            j = self.data_Cerate
            for i in range(j):
                data.append(self.readByte())


            Verification =  self.readByte()


            self.reply = {


                'Length' :Length,
                'Deveice_ID' :Deveice_ID,
                'Command_code' :Command_code,
                'Status' :Status,
                'data' :data,
                'Verification' :Verification
            }

            return self.reply

    def verification_CMD(self,length):


        verification_xor = 0

        for i in range (length-4):
            verification_xor = operator.xor(verification_xor,ord(self.CMD[3+i]))

        verification_xor = verification_xor & 0xff
        self.CMD[length-1] = chr(verification_xor)




    def length(self,length):

        length_sum = length - 4
        length_sum = length_sum & 0xffff
        self.CMD[2] = chr(length_sum)


    def Create_CMD_Head(self,length):

        self.CMD = []
        for i in range (length):
             self.CMD.append(chr(0))

        self.CMD[0] = chr(0xAA)
        self.CMD[1] = chr(0xBB)



    def CMD_KeyA(self):

        self.CMD[10] = chr(0xFF)
        self.CMD[11] = chr(0xFF)
        self.CMD[12] = chr(0xFF)
        self.CMD[13] = chr(0xFF)
        self.CMD[14] = chr(0xFF)
        self.CMD[15] = chr(0xFF)


    def ping(self):
        self.debug("ping")
        length_CMD = 9

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x04)
        self.CMD[7] = chr(0x01)

        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()


    def beep(self):
        self.debug("beep")
        length_CMD = 10

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x06)
        self.CMD[7] = chr(0x01)
        self.CMD[8] = chr(0x10)

        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()

    def request(self):
##        self.debug("request")
        length_CMD = 10

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x01)
        self.CMD[7] = chr(0x02)
        self.CMD[8] = chr(0x26)

        self.verification_CMD(length_CMD)
        self.length(length_CMD)


        self.writeCommand(self.CMD)
##        self.result()

        a = self.result()
        return(a)



    def anticoll(self):
##        self.debug ("anticoll")
        length_CMD = 9

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x02)
        self.CMD[7] = chr(0x02)

        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()



    def select(self):
##        self.debug ("select")
        length_CMD = 13

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x03)
        self.CMD[7] = chr(0x02)
        self.CMD[8] = chr(self.Card_serial_Num[0])
        self.CMD[9] = chr(self.Card_serial_Num[1])
        self.CMD[10] = chr(self.Card_serial_Num[2])
        self.CMD[11] = chr(self.Card_serial_Num[3])

        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()


    def init_type(self):
##        debug "init type"
        length_CMD = 10

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x08)
        self.CMD[7] = chr(0x01)
        self.CMD[8] = chr(0x41)
        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()

    def antenna_sta(self,Parameter):
 ##       debug "antenna_sta"
        length_CMD = 10


        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x0C)
        self.CMD[7] = chr(0x01)
        self.CMD[8] = chr(Parameter)
        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()

    def authen(self,block):
##        self.debug ("authen")
        length_CMD = 17

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x07)
        self.CMD[7] = chr(0x02)
        self.CMD[8] = chr(0x60)
        self.CMD[9] = chr(block)

        self.CMD_KeyA()
        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()


    # ==================================================
    # Read function
    # ==================================================


    def read_data(self):
        # Sector 0, Block 1
        self.result_read_string = 0
        self.request()
        self.anticoll()
        self.select()
        self.authen(1)
        self.debug("Read data")
        length_CMD = 10

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x08)
        self.CMD[7] = chr(0x02)
        self.CMD[8] = chr(0x01)

        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()

        return(self.data)

    def read_String(self):
        # Sector 0, Block 1

        self.result_read_string = 1
        if self.request() == 0:
            self.debug("Not Read : Tag not found")
            return("''")
        else:
            self.anticoll()
            self.select()
            self.authen(1)
##            self.debug("Read String")
            length_CMD = 10

            self.Create_CMD_Head(length_CMD)
            self.CMD[6] = chr(0x08)
            self.CMD[7] = chr(0x02)
            self.CMD[8] = chr(0x01)

            self.verification_CMD(length_CMD)
            self.length(length_CMD)

            self.writeCommand(self.CMD)
            self.result()

            return(self.data_String)

    def SectorToBlock(self,sector,block):

        if sector <= 15 and block <= 3:

            if sector == 0 and block == 0:
                self.debug("Manufacturer Data Block! is not read/write")
                sys.exit()
            if block == 3:
                self.debug("Key Block! is not read/write")
                sys.exit()

            if sector == 1:
                block = block + 4
            elif sector == 2:
                block = block + 8
            elif sector == 3:
                block = block + 12
            elif sector == 4:
                block = block + 16
            elif sector == 5:
                block = block + 20
            elif sector == 6:
                block = block + 24
            elif sector == 7:
                block = block + 28
            elif sector == 8:
                block = block + 32
            elif sector == 9:
                block = block + 36
            elif sector == 10:
                block = block + 40
            elif sector == 11:
                block = block + 44
            elif sector == 12:
                block = block + 48
            elif sector == 13:
                block = block + 52
            elif sector == 14:
                block = block + 56
            elif sector == 15:
                block = block + 60

            self.block_return = block

        elif sector > 15:
            self.debug(">>RFID read error")
            self.debug(">>not Sector")
            sys.exit()

        elif block > 3:
            self.debug(">>RFID read error")
            self.debug(">>not Block")
            sys.exit()


    def read(self,sector,block):

        self.result_read_string = 0

        self.debug("Sector %d Block %d" % (sector ,  block))

        self.SectorToBlock(sector,block)

        self.request()
        self.anticoll()
        self.select()
        self.authen(self.block_return)
        self.debug("read")
        length_CMD = 10

        self.Create_CMD_Head(length_CMD)
        self.CMD[6] = chr(0x08)
        self.CMD[7] = chr(0x02)
        self.CMD[8] = chr(block)

        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()


    # ==================================================
    # Write function
    # ==================================================


    def write_String(self,data):
        #Sector 0, Block 1

       if self.request() == 0:
            self.debug("Not write : Tag not found")
            return("''")
       else:

            self.anticoll()
            self.select()
            self.authen(1)
            self.debug("Write")

            length_CMD = 26
            self.Create_CMD_Head(length_CMD)

            if len(data) <= 16:
                for i in range(len(data)):
                    self.CMD[9+i] = data[i]

                self.CMD[6] = chr(0x09)
                self.CMD[7] = chr(0x02)
                self.CMD[8] = chr(0x01)

                self.verification_CMD(length_CMD)
                self.length(length_CMD)

                self.writeCommand(self.CMD)
                self.result()

            else:
                self.debug("Write error: data more 16 characters")


    def write_data(self,data):

        self.request()
        self.anticoll()
        self.select()
        self.authen(1)
        self.debug("Write")

        length_CMD = 26
        self.Create_CMD_Head(length_CMD)            #CMD[0] , CMD[1]


        self.CMD[6] = chr(0x09)
        self.CMD[7] = chr(0x02)
        self.CMD[8] = chr(0x01)

        for i in range(16):
            self.CMD[9+i] = chr(data[i])


        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()


    def write(self,sector,block,data):


        self.debug("Sector %d Block %d" % (sector , block))

        self.SectorToBlock(sector,block)

        self.request()
        self.anticoll()
        self.select()
        self.authen(self.block_return)

        self.debug("Write")


        length_CMD = 26
        self.Create_CMD_Head(length_CMD)            #CMD[0] , CMD[1]


        self.CMD[6] = chr(0x09)
        self.CMD[7] = chr(0x02)
        self.CMD[8] = chr(0x01)

        for i in range(16):
            self.CMD[9+i] = chr(data[i])


        self.verification_CMD(length_CMD)
        self.length(length_CMD)

        self.writeCommand(self.CMD)
        self.result()


    # ==================================================
    # Result command
    # ==================================================


    def result(self):

##        RFIDTimeOut = 0

##        while (self.ser.inWaiting() == 0):
##            time.sleep(0.01)
##
##            #Loop RFIDTimeOut 15s to exit Program and Cancel for FP
##            if RFIDTimeOut >= 1500:
##                debug ">>RFID Time Out"
##                RFIDTimeOut = 0
##                sys.exit()
##
##
##            RFIDTimeOut += 1
##
##
##        RFIDTimeOut = 0

        result = self.readReply()

        #Result Code : 0 = Succeess and data code
        if result['Status'] == 0x00:
##            self.debug (">>Success")
            #result Card serial Number 0x0202
            if result['Command_code'] == 0x0202:
                self.Card_serial_Num = [4]
                for i in range (4):
                    self.Card_serial_Num.append(chr(0))

                self.Card_serial_Num[0] = result['data'][0]
                self.Card_serial_Num[1] = result['data'][1]
                self.Card_serial_Num[2] = result['data'][2]
                self.Card_serial_Num[3] = result['data'][3]

##                debug result['data']

            # result Read data 0x0802
            elif result['Command_code'] == 0x0208:
                # Read String data
                if self.result_read_string == 1:


                    j = 0

                    for i in range (16):
                        if result['data'][i] != 0x00 :
                            j += 1

                    # Tag not Empty retrun data
                    if j != 0:
                        data_Array  = [j]
                        for i in range (j-1):
                            data_Array .append(chr(0))

                        for i in range (j):
                            data_Array [i] = chr(result['data'][i])

                        # function Array to String
                        self.data_String = ''.join(data_Array)

##                        self.debug(self.data_String)
        ##                    if data_Staring == "BANK":
        ##                        debug "OK"
                        return(self.data_String)

                    # Tag is Empty retrun ""
                    else :
                        self.data_String = "''"
                        self.debug("Tag is Empty")
                        return(self.data_String)


                # Read data
                else:

                    self.data = result['data']
                    self.debug(self.data)
                    return(self.data)

                self.result_read_string = 0

        #Result Code : not 0 = fail
        elif result['Status'] != 0x00:
            if result['Command_code'] == 0x0201:
##                self.debug(">>request fail")
                return(0)

##            if result['Command_code'] == 0x0208:
##                self.debug(" ")
##                return(0)
        else:
            return(0)




if __name__ == '__main__':

    # ==================================================
    # main
    # ==================================================


    rf = RFID_Reader()
##    rf.connect("/dev/ttyUSB0")
    rf.connect(7)

##    rf.ping()
##    rf.beep()
##    time.sleep(0.5)

try:
    print "Read string = " + rf.read_String()
    rf.write_String("ABC")
    print "Re-Read = " + rf.read_String()
except:
    print "Tag not found"
    ##---------------------------------------------------------------


    ##if RF.read_String() == "ATTAPAN":
    ##    RF.beep()
    ##elif RF.read_String() == "BANK":
    ##
    ##    RF.beep()
    ##    time.sleep(0.2)
    ##    RF.beep()
    ##elif RF.read_String() == "LIL@CMU":
    ##
    ##    RF.beep()
    ##    time.sleep(0.2)
    ##    RF.beep()
    ##    time.sleep(0.2)
    ##    RF.beep()

    ##-----------------------------------------------------

##	if RF.read_data() == [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2]:
##		self.debug("OK")
##		RF.beep()
##
##	else:
##		self.debug("fail")



    ##-----------------------------------------------------
##try:
##    rf.read_String()
##except:
##    print "Tag not found"
