#/usr/bin/env python

#Python network access to P7SPIder or TeensySPIder MCU via TCP/IP

import socket
import time
import urllib.parse

#when using defaultHostName - not none - we do not need to know IP address:
defaultHostName = None
#defaultHostName = "P7MCU01"     #use NetBIOS and device name
if defaultHostName != None :
    defaultHostIPaddress = "0.0.0.0"
else :
    #MCU IP address, use "ipaddr" command on UART
    ##defaultHostIPaddress = "10.59.144.26"     #default STATIC
    ##defaultHostIPaddress = "192.168.0.84"     #my DHCP I get
    defaultHostIPaddress = "192.168.1.169"      #MCU STATIC IP address

##TCPport = 80                #the port for commands, web browser HTTP request port
TCPport = 8080
maxTCPLen = 64*4*1024       #the maximum expected packet length (same or larger as largest response)

#the maximum TCP packet size the MCU can handle is: 64*4*1024 +32 (with the 4 byte header),
#so, the max. payload size is: 64*4*1024 +32 - 4
#even the MCU will check the length values - make sure not to send a larger 'as possible' packet

#the Python script leaves the TCP socket open until it ends:
#it does not create a new socket - otherwise we have issue with PORT number after 64K port numbers used:
#the old PORTs remain in 'zombie' state for a while and host cannot create a new socket

class NetworkInterface() :
    """Our Network Object to interface MCU via TCP/IP commands"""

    def __init__(self, IPAddress=defaultHostIPaddress) :
        global TCPport
        global defaultHostName

        self.host = IPAddress
        self.port = TCPport
        self.hostname = defaultHostName
        #resolve IP address by name - but just if DHCP is used (see on top)
        if self.hostname != None :
            self.host = socket.gethostbyname(self.hostname)
        self.sock = None

    def ConnectOpen(self) :
        if self.sock == None:
            self.sock = socket.create_connection((self.host, self.port))
            self.sock.setblocking(1)

    def ConnectClose(self) :
        if self.sock != None:
            self.sock.close()
            self.sock = None

    def stripOfHeader(self, data) :
        return data[105:]

    def TextCommand(self, cmd) :
        """ATTENTION: this TextCommand() supports only up to 1460 ASCII characters total,
           for longer ASCII commands - use TextCommandBinary()
           This command sends like regular HTTP request, with a "GET /..." ASCII string
           ATTENTION: the socket is closed after one request!
        """
        global maxTCPLen
        #ATT: we need 2x NEWLINE at the end! Web Server expects it as end_of_request

        #encode as URL:
        #we have to encode space and other characters as an URL!
        ##cmd = urllib.parse.quote(cmd)

        message = cmd     #we send without 'GET /'! we do not need here 2x \r\n
        ##print("\nLen message: %d" % (len(message)))
        ##self.ConnectClose()
        self.ConnectOpen()

        self.sock.sendall(message.encode())
        ##print(message)
        ##print("\nLen receive: %d" % (maxTCPLen))
        data = self.sock.recv(maxTCPLen)
        while True : 
            if b"\003" in data :
                ##print("BREAK")
                break
            ##print("next...")
            data = data + self.sock.recv(maxTCPLen)

        ##print("\nLen response: %d" % (len(data)))
        ##print(data)
        data = data.decode()
                 
        #print response for the command

        ##we get the reponse with a HTML header: strip it off:
        ##data = self.stripOfHeader(data)

        print(data)

        #close connection as web browser would do
        self.ConnectClose()

    def TextCommand2(self, cmd) :
        """ATTENTION: this TextCommand() supports only up to 1460 ASCII characters total,
           including the string GET /C !
           for longer ASCII commands - use TextCommandBinary()
           This command sends like regular HTTP request, with a "GET /C..." ASCII string
           ATTENTION: the socket is closed after one request!
        """
        global maxTCPLen
        #ATT: we need 2x NEWLINE at the end! Web Server expects it as end_of_request

        #encode as URL:
        #we have to encode space and other characters as an URL!
        ##cmd = urllib.parse.quote(cmd)

        message = 'GET /C' + cmd
        ##print("\nLen message: %d" % (len(message)))
        ##self.ConnectClose()
        self.ConnectOpen()

        self.sock.sendall(message.encode())
        ##print(message)
        ##print("\nLen receive: %d" % (maxTCPLen))
        data = self.sock.recv(maxTCPLen)
        while True : 
            if b"\003" in data :
                ##print("BREAK")
                break
            ##print("next...")
            data = data + self.sock.recv(maxTCPLen)

        ##print("\nLen response: %d" % (len(data)))
        ##print(data)
        data = data.decode()
                 
        #print response for the command

        ##we get the reponse with a HTML header: strip it off:
        ##data = self.stripOfHeader(data)

        print(data)

        #close connection as web browser would do
        self.ConnectClose()

    def TextCommandBinary(self, cmd) :
        """TextCommandBinary() send an ASCII command via BINARY: CMD 0x01 plus LEN... as 3 bytes
           where LEN is the total length of following in binary, as LITTLE_ENDIAN
           This supports a much longer single line command string, up to max. TCP packet length - overhead.
           REMARK: the socket remains open after this request
        """
        global maxTCPLen
        lenCmd  = len(cmd)          #without NL at the end
        #code for BINARY command (with LEN, but as ASCII text)
        message = bytearray(lenCmd + 4)
        message[0] = 0x01
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length
        for i in range(lenCmd) :
            message[4 + i] = ord(cmd[i])

        #print(repr(message))

        self.ConnectOpen()      #open if not still open

        self.sock.sendall(message)
        data = self.sock.recv(maxTCPLen)
                 
        #print response for the command
        ##print(repr(data))
        ##print("len resp: %d" % (len(data))) 
        if (len(data) >= 4) :        #actually, we should have always a response from server, lenRsp can be zero
            lenRsp = data[1] << 0
            lenRsp = lenRsp | (data[2] << 8)
            lenRsp = lenRsp | (data[3] >> 16)
            ##print("exp. resp len: %d" % lenRsp)
            while (len(data) < (lenRsp + 4)) :
                ##print("wait: %d" % (lenRsp + 4))
                data = data + self.sock.recv(maxTCPLen)
            if lenRsp > 0 :
                print(data[4:].decode())

        #let the connection open
        #self.ConnectClose()

    def TextCommandBinary2(self, cmd) :
        """TextCommandLarge() send an ASCII command via BINARY: "GET /c plus LEN... 
           where LEN is the total length of following in binary, as BIG_ENDIAN, two bytes
           This supports a much longer single line command string, up to max. TCP packet length - overhead.
           REMARK: the socket remains open after this request
        """
        global maxTCPLen
        lenCmd  = len(cmd)          #without NL at the end
        #code for BINARY command (with LEN, but as ASCII text)
        message = bytearray(lenCmd + 8)
        message[0] = ord('G')
        message[1] = ord('E')
        message[2] = ord('T')
        message[3] = ord(' ')
        message[4] = ord('/')
        message[5] = ord('c')
        message[6] = (lenCmd >>  8) & 0xFF  #BIG ENDIAN
        message[7] = (lenCmd >>  0) & 0xFf
        for i in range(lenCmd) :
            message[8 + i] = ord(cmd[i])

        #print(repr(message))

        self.ConnectOpen()      #open if not still open

        self.sock.sendall(message)
        data = self.sock.recv(maxTCPLen)
                 
        print(data[0:].decode())

        #let the connection open
        #self.ConnectClose()

    def CommandShell(self) :
        """interactive, like TELNET, shell command, using the implementations for commands
           quit the loop with single character q typed
        """
        cmd = ""

        ##self.ConnectOpen()    #we do anyway in TextCmmandBinary etc.
        while cmd != 'q':
            cmd = input("-> ")
            if cmd == 'q':
                break;

            ##print(cmd)
            #use TEXT vs. BINARY command (a binary packet with ASCII command text)

            ##self.TextCommand(cmd)
            ##self.TextCommand2(cmd)
            ##self.TextCommandBinary(cmd)
            self.TextCommandBinary2(cmd)

        #close the connection when done
        self.ConnectClose()
        print ('Good bye')

    def TestLargeTCPCommand(self) :
        cmd = "print ==== TEST====; spitr 0xf106 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000 0x0000; spitr 0xe002 0x0000; cid; print ==== END ===="
        self.TextCommandBinary2(cmd)

    def SPITransactionBinary(self, spipkt) :
        global maxTCPLen
        """send a binary raw SPI transaction
           ATTENTION: the max. binary length is limited to 64*4*1024 bytes
           REMARK: the socket remains open after the request
        """
        lenCmd = len(spipkt)
        message = bytearray(lenCmd + 4)

        message[0] = 0x02
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length
        #copy the SPI transaction bytes
        for i in range(lenCmd) :
            message[4 + i] = spipkt[i] & 0xFF

        ##print("len tx: %d" % (lenCmd + 4))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        data = b''
        while len(data) < (lenCmd + 4) :
            data = data + self.sock.recv(maxTCPLen)

        #option: we could evaluate the length in response, should be the same
        l = data[1] << 0
        l = l | data[2] << 8
        l = l | data[3] << 16
        ##print("len rx: %d" % (l))
        
        #let the connection open
        ##self.ConnectClose()
        return data[4:]

    def MultiSPITransactionBinary(self, listOfSPI) :
        global maxTCPLen
        """send several binary raw SPI transaction
           after binary total length, every SPI transaction starts with 2 byte, Little Endian
           for the SPI transaction
           listOfSPI is a list of the SPI transactions
        """
        #we have to build the total length of all SPI transactions in list
        lenCmd = 0
        for i in range(len(listOfSPI)) :
            lenCmd = lenCmd + len(listOfSPI[i]) + 2

        message = bytearray(lenCmd + 4)

        message[0] = 0x03
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length
        #copy the SPI transaction bytes - generate a 2 byte length and SPI transaction
        idx = 0
        for i in range(len(listOfSPI)) :
            l = len(listOfSPI[i])
            message[4 + idx + 0] = (l >> 0) & 0xFF
            message[4 + idx + 1] = (l >> 8) & 0xFF
            for j in range(l) :
                message[4 + 2 + idx + j] = listOfSPI[i][j] & 0xFF
            idx = idx + 2 + l

        ##print("Total Len: %d" % (lenCmd))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        data = b''
        while len(data) < (lenCmd + 4) :
            data = data + self.sock.recv(maxTCPLen)

        #option: we could evaluate the length in response, should be the same
        l = data[1] << 0
        l = l | data[2] << 8
        l = l | data[3] << 16
        ##print("len rx: %d" % (l))
        
        #build a list of bytearrays as response - we use the input SPI transactions and length reference
        idx = 0
        for i in range(len(listOfSPI)) :
            l = len(listOfSPI[i])
            for j in range(l) :
                listOfSPI[i][j] = data[4+2+idx+j]
            idx = idx + l + 2

        ##self.ConnectClose()
        return listOfSPI

    def DummyTransactionBinary(self, spipkt) :
        global maxTCPLen
        """this sends a non-existing command, the MCU will respond with the same packet back
           we use in order to measure the network round trip delay
        """
        lenCmd = len(spipkt)
        message = bytearray(lenCmd + 4)

        message[0] = 0x1F
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length
        #copy the SPI transaction bytes
        for i in range(lenCmd) :
            message[4 + i] = spipkt[i] & 0xFF

        ##print("Dummy Transaction: len tx: %d" % (lenCmd))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        data = b''
        while len(data) < (lenCmd + 4) :
            data = data + self.sock.recv(maxTCPLen)

        #let the connection open
        ##self.ConnectClose()

    def I2CReadTransactionBinary(self, slaveaddr, regaddr, numbytes) :
        global maxTCPLen
        """do an I2C memory read transaction (register read, i2crr)
        """
        lenCmd = 3          #slaveaddr, regaddr, numbytes as parameters for command
        message = bytearray(lenCmd + 4)

        message[0] = 0x05                   #binary I2C read memory (register)
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length
        #place the I2C command parameters: slaveaddr, regaddr, numbytes
        message[4] = slaveaddr
        message[5] = regaddr
        message[6] = numbytes

        #command length is 4 + 3 bytes
        ##print("len tx: %d" % (lenCmd))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        #the response is the bytes read and length is set to it
        #ATT: even command fails - MCU sets length to 0
        data = b''
        data = data + self.sock.recv(maxTCPLen)

        l = data[1] << 0
        l = l | (data[2] << 8)
        l = l | (data[3] << 16)
        ##print("len rx: %d" % (l))
        
        while len(data) < (l + 4) :
            data = data + self.sock.recv(maxTCPLen)
        
        #let the connection open
        ##self.ConnectClose()
        return data[4:]

    def I2CWriteTransactionBinary(self, slaveaddr, i2cpkt) :
        global maxTCPLen
        """send a binary I2C write transaction (register write, i2cwr)
           ATTENTION: the register address is the 1st byte in i2cpkt
        """
        lenCmd = len(i2cpkt)                #ATT: later + 1 for slaveaddr
        message = bytearray(lenCmd + 4 + 1) #space for slaveaddr in command

        message[0] = 0x04                   #binary I2C write
        message[1] = ((lenCmd +1) >>  0) & 0xFF
        message[2] = ((lenCmd +1) >>  8) & 0xFf
        message[3] = ((lenCmd +1) >> 16) & 0xFF  #LITTLE ENDIAN length
        #copy the I2C transaction bytes - the first is slaveaddr
        message[4] = slaveaddr              #slaveaddr first
        for i in range(lenCmd) :            #1st byte is regAddr
            message[5 + i] = i2cpkt[i] & 0xFF

        ##print("Len: %d" % (lenCmd + 1))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        #we get the same back as response or len 0 on Error
        data = b''
        data = data + self.sock.recv(maxTCPLen)

        l = data[1] << 0
        l = l | data[2] << 8
        l = l | data[3] << 16
        ##print("len rx: %d" % (l))

        while len(data) < (l + 4) :
            data = data + self.sock.recv(maxTCPLen)
        
        #let the connection open
        ##self.ConnectClose()
        #it returns the same parameter buffer if all wass OK
        return data[4:]

    #------------ chip specific commands ---------------------------
    #they depend on which Chip Number is selected in syscfg on MCU 
    #they take arguments like: register address (32bit), a number for words (registers) (32bit) to read
    #or to provide register values (as chip-dependent, e.g. 16bit words) to write a list of values
    #the read functions return a list of values

    def Chip_RREG(self, addr, numWords) :
        global maxTCPLen
        """send a binary SPI transaction for RREG
           ATTENTION: the max. numWords is limited to (64*4*1024 - 4)/2
           it reads numWords as 16bit values (LITTLE ENDIAN) via SPI from chip
        """
        lenCmd = 8                          #we have 32bit addr plus 32bit numWords
        message = bytearray(lenCmd + 4)

        message[0] = 0x06                   #RREG command
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length payload

        message[4] = (addr >>  0) & 0xFF
        message[5] = (addr >>  8) & 0xFF
        message[6] = (addr >> 16) & 0xFF
        message[7] = (addr >> 24) & 0xFF    #LITTLE ENDIAN addr

        message[8]  = (numWords >>  0) & 0xFF
        message[9]  = (numWords >>  8) & 0xFF
        message[10] = (numWords >> 16) & 0xFF
        message[11] = (numWords >> 24) & 0xFF   #LITTLE ENDIAN numWords

        ##print("len tx: %d" % (lenCmd + 4))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        #we get the same back as response or len 0 on Error
        data = b''
        data = data + self.sock.recv(maxTCPLen)

        l = data[1] << 0
        l = l | data[2] << 8
        l = l | data[3] << 16
        ##print("len rx: %d" % (l))

        while len(data) < (l + 4) :
            data = data + self.sock.recv(maxTCPLen)

        #let the connection open
        ##self.ConnectClose()

        #convert the response with numWords * 16bit words (LITTLE ENDIAN) into a list
        listOfWords = [];

        for i in range(l // 2) :
            val = data[4 + 2*i + 0]
            val = val | (data[4 + 2*i + 1] << 8)
            listOfWords.append(val)

        return listOfWords

    def Chip_RBLK(self, addr, numBytes) :
        global maxTCPLen
        """send a binary SPI transaction for RBLK
           ATTENTION: the max. numBytes is limited to (64*4*1024 - 4)
           it reads numBytes as 8bit values (Bytes) via SPI from chip
           and returns a listOfBytes
        """
        lenCmd = 8                          #we have 32bit addr plus 32bit numWords
        message = bytearray(lenCmd + 4)

        message[0] = 0x08                   #RBLK command
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length payload

        message[4] = (addr >>  0) & 0xFF
        message[5] = (addr >>  8) & 0xFF
        message[6] = (addr >> 16) & 0xFF
        message[7] = (addr >> 24) & 0xFF    #LITTLE ENDIAN addr

        message[8]  = (numBytes >>  0) & 0xFF
        message[9]  = (numBytes >>  8) & 0xFF
        message[10] = (numBytes >> 16) & 0xFF
        message[11] = (numBytes >> 24) & 0xFF   #LITTLE ENDIAN numBytes

        ##print("len tx: %d" % (lenCmd + 4))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        #we get the same back as response or len 0 on Error
        data = b''
        data = data + self.sock.recv(maxTCPLen)

        l = data[1] << 0
        l = l | data[2] << 8
        l = l | data[3] << 16
        ##print("len rx: %d" % (l))

        while len(data) < (l + 4) :
            data = data + self.sock.recv(maxTCPLen)

        #let the connection open
        ##self.ConnectClose()

        #convert the response with numBytes as 8bit Bytes into a list
        listOfBytes = bytearray(l);

        for i in range(l) :
            listOfBytes[i] = data[4 + i]

        return listOfBytes

    def Chip_NOOP(self, numWords) :
        global maxTCPLen
        """send a binary SPI transaction for N numbers of NOOP done on SPI 
        """
        lenCmd = 4                          #we have 32bit as numWords
        message = bytearray(lenCmd + 4)

        message[0] = 0x0A                   #RBLK command
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length payload

        message[4]  = (numWords >>  0) & 0xFF
        message[5]  = (numWords >>  8) & 0xFF
        message[6] = (numWords >> 16) & 0xFF
        message[7] = (numWords >> 24) & 0xFF   #LITTLE ENDIAN numWords

        ##print("len tx: %d" % (lenCmd + 4))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        data = self.sock.recv(maxTCPLen)

        #we could evaluate response and payload length, but should be zero anyway

    def Chip_WREG(self, addr, listOfWords) :
        global maxTCPLen
        """send a binary SPI transaction for WREG
           ATTENTION: the max. numWords is limited to (64*4*1024 - 4)/2
        """
        lenCmd = len(listOfWords) * 2       #we have N number of words, here as 16bit values
        message = bytearray(lenCmd + 8)     #header (4 byte) plus addr (4 byte)

        message[0] = 0x07                   #WREG command
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length payload

        message[4] = (addr >>  0) & 0xFF
        message[5] = (addr >>  8) & 0xFF
        message[6] = (addr >> 16) & 0xFF
        message[7] = (addr >> 24) & 0xFF    #LITTLE ENDIAN addr

        #place the listOfWords into payload
        for i in range(len(listOfWords)) :
            val = listOfWords[i]
            message[8 + 2*i + 0] = (val >> 0) & 0xFF
            message[8 + 2*i + 1] = (val >> 8) & 0xFF    #LITTLE ENDIAN value

        ##print("len tx: %d" % ((i+1)*2 + 8))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        data = self.sock.recv(maxTCPLen)

        #we could evaluate response and payload length, but should be zero anyway

    def Chip_WBLK(self, addr, listOfBytes) :
        global maxTCPLen
        """send a binary SPI transaction for WBLK
           it writes given numberBytes as bytes in listOfBytes
           ATTENTION: the max. numBytes is limited to (64*4*1024 - 4)
        """
        lenCmd = len(listOfBytes)           #we have N number of words, here as 16bit values
        message = bytearray(lenCmd + 8)     #header (4 byte) plus addr (4 byte)

        message[0] = 0x09                   #WBLK command
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length payload

        message[4] = (addr >>  0) & 0xFF
        message[5] = (addr >>  8) & 0xFF
        message[6] = (addr >> 16) & 0xFF
        message[7] = (addr >> 24) & 0xFF    #LITTLE ENDIAN addr

        #place the listOfWords into payload
        for i in range(len(listOfBytes)) :
            message[8 + i] = listOfBytes[i]

        ##print("len tx: %d" % ((i+1)*2 + 8))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        data = self.sock.recv(maxTCPLen)

        #we could evaluate response and payload length, but should be zero anyway

#----------- Matlab wrapper ---------------------------------------------------

netObj = None

def NetworkInit() :
    global netObj
    netObj = NetworkInterface()
    return 1            #as OK

def NetworkClose() :
    global netObj
    if netObj != None :
        netObj.ConnectClose()

def read_reg_single(addr) :
    global netObj
    if netObj != None :
        r = netObj.Chip_RREG(addr, 1)
        return int(r[0])
    else :
        return int(0)

def write_reg_single(addr, value) :
    global netObj
    if netObj != None :
        netObj.Chip_WREG(addr, [value])
        return 1        #as OK
    else :
        return 0        #as ERROR

def read_reg_multiple(addr, num) :
    global netObj
    if netObj != None :
        r = netObj.Chip_RREG(addr, num)
        return r
    else :
        return 

def write_reg_multiple(addr, listVal) :
    global netObj
    if netObj != None :
        netObj.Chip_WREG(addr, listVal)
        return 1        #as OK
    else :
        return 0        #as ERROR

def read_memory(addr, num) :
    global netObj
    if netObj != None :
        r = netObj.Chip_RBLK(addr, num)
        return r
    else :
        return 

def write_memory(addr, listBytes) :
    global netObj
    if netObj != None :
        netObj.Chip_WBLK(addr, listBytes)
        return 1        #as OK
    else :
        return 0        #as ERROR

def convert12bitToBytes(listOf12bit) :
    """ convert an array with 12bit values to list of Bytes
        we shift the nibbles for "LSB first"
        the byte sequence is "LSB first"
        call it always with 2 * N 12bit values
    """
    N = len(listOf12bit)    #how many 12bit values provided
    
    b = bytearray((N*3) // 2)
    L = len(b)

    i = 0       #byte index
    j = 0       #12bit word index
    while L > 0 :
        b[i] = listOf12bit[j] & 0xFF                    #LSB first
        b[i + 1] = (listOf12bit[j] & 0xF00) >> 8        #MSB now, but as LSB in byte
        b[i + 1] = b[i + 1] | ((listOf12bit[j + 1] & 0xF) << 4)  #LSB first again
        b[i + 2] = (listOf12bit[j + 1] & 0xFF0) >> 4    #MSB now
        #move the index now
        i = i + 3                                       #byte index moves by 3
        j = j + 2                                       #12bit index moves by 2
        #how often to do the loop? over entire byte array
        L = L - 3

    return b

def convertBytesTo12bit(listOfBytes) :
    """ convert byte array to list of 12bit values
        we take the nibbles as "LSB first"
        the byte sequence is "LSB first"
        it returns the 12bit values as a list
    """
    L = len(listOfBytes)

    #create list of 12bit values
    v = [0] * ((L * 2) // 3)

    i = 0       #byte index
    j = 0       #12bit word index
    while (L > 0) :
        v[j] = int(listOfBytes[i])                          #the LSB first
        v[j] = v[j] | int((listOfBytes[i + 1] & 0xF) << 8)  #MSB now, but as LSB in byte
        v[j + 1] = int((listOfBytes[i + 1] & 0xF0) >> 4)    #LSB first again
        v[j + 1] = v[j + 1] | int(listOfBytes[i + 2] << 4)  #MSB now
        #move the index now
        i = i + 3
        j = j + 2
        #how often to do the loop? over entire byte array
        L = L - 3

    return v

def write_memory_12bitValues(addr, listOf12bit) :
    x = convert12bitToBytes(listOf12bit)

    #debug: check if byte sequence looks correct
    print("D: {}".format(x.hex()))
    #now send the byte array as a WBLK command
    write_memory(addr, x)

def read_memory_12bitValues(addr, num12bitWords) :
    #read the memory first via RBLK command
    x = read_memory(addr, (num12bitWords * 2 ) // 3)

    #debug: check if byte sequence looks correct (as hex stream)
    print("d: {}".format(x.hex()))
    #now convert byte array back to 12bit values
    y = convertBytesTo12bit(x)

    #debug: check if the 12bit values look correct (as hex)
    print("--- 12bit values:")
    for i in y : print("{} ".format(hex(i)), end = '')

    return y

##TODO:
##add similar wrapper functions to read/write memory blocks

#----------- Python standalone ------------------------------------------------
#test sequence:

def Main() :
    global defaultHostIPaddress
    global TCPport
    global maxTCPLen
    
    mcuNet = NetworkInterface()

    ##maxTCPLen = (64*1024 + 16)
    b = bytearray(maxTCPLen)
    #this is ReadChipID - not working on CHIP-2
    for i in range(maxTCPLen // 4):
        b[0 + i*4] = 0x02
        b[1 + i*4] = 0xE0
        b[2 + i*4] = 0x11
        b[3 + i*4] = 0x22

    if 0 :
        avg = 0;
        for i in range(10) :
            start_time = time.time()
            r = mcuNet.DummyTransactionBinary(b)
            end_time = time.time()
            print("--- %s seconds ---" % (end_time - start_time))
            avg = avg + (end_time - start_time)
        print("\n*** AVERAGE: %s seconds ***\n" % (round(avg / 10, 3)))

    if 0 :
        start_time = time.time()
        r = mcuNet.SPITransactionBinary(b)
        end_time = time.time()

        ##print("resp len: %d | %d" % (maxTCPLen, len(r)))

        if 0 :
            for i in range(len(r)):
                print("%2.2x " % (r[i]), end='')
            print()

        print("--- %s seconds ---\r\n" % (end_time - start_time))

    if 0:
        #multi-SPI transaction test
        spi1 = bytearray(24)
        #create ReadChiPID command
        spi1[0] = 0x80; spi1[1] = 0x05; spi1[2] = 0x04; spi1[3] = 0x00
        spi1[4] = 0xD0; spi1[5] = 0x0F; spi1[6] = 0x0B; spi1[7] = 0x40
        spi1[8] = 0x00; spi1[9] = 0x00; spi1[10] = 0xA0; spi1[11] = 0xAA
        spi1[12] = 0x00; spi1[13] = 0x00; spi1[14] = 0x00; spi1[15] = 0x00
        spi1[16] = 0x00; spi1[17] = 0x00; spi1[18] = 0x00; spi1[19] = 0x00
        spi1[20] = 0x00; spi1[21] = 0x00; spi1[22] = 0x00; spi1[23] = 0x00

        #send it 3x as back-to-back
        r = mcuNet.MultiSPITransactionBinary([spi1, spi1, spi1, spi1, spi1])

        #print response
        for i in range(len(r)) :
            ##print(repr(r[i]))
            for j in range(len(r[i])):
                print("%2.2x " % (r[i][j]), end='')
            print()

    if 0 :
        #test I2C memory read binary command
        #ATT: we use the Portenta H7 internal I2C on PMIC chip to try
        r = mcuNet.I2CReadTransactionBinary(0x10, 0x0, 1)
        #print response
        for i in range(len(r)) :
            print(repr(r[i]))

    if 0:
        #test the I2C write binary command
        #ATT: the data bytes contain also as first one the register address
        i2cpkt = bytearray(2);          #1st byte is register address! plus 3 bytes to write
        i2cpkt[0] = 0x9E;
        i2cpkt[1] = 0x20;

        r = mcuNet.I2CWriteTransactionBinary(0x10, i2cpkt)

        #print response - on write it should be the same as provided
        if 0 :
            for i in range(len(r)) :
                print(repr(r[i]))

    if 0 :
        #test the maximum length of an ASCII text command
        mcuNet.TextCommand("help")
        cmd = "print "
        for i in range(8100 - 6) :
            cmd = cmd + "X"
        ##mcuNet.TextCommand(cmd)
        mcuNet.TextCommandBinary(cmd)
        for i in range(6000000) :
            mcuNet.TextCommand("print aaaaa")

    #chip sepcific command test:
    if 0 :
        #read register <addr> for <numRegisters>
        listOfRegVal = mcuNet.Chip_RREG(0x0, 16)
        print(listOfRegVal)

    if 0 :
        #read RAM (block memory) <addr> for <numWords>
        listOfBlkVal = mcuNet.Chip_RBLK(0, 32)
        print(listOfBlkVal)

    if 0 :
        #generate <num> NOOP on SPI (as a NOOP command)
        mcuNet.Chip_NOOP(5)

    if 0 :
        #write register <addr> <value ...> (number of list elements)
        mcuNet.Chip_WREG(0x0002, [1, 2, 3, 4, 5, 6, 7])

    if 0 :
        #write RAM (block memory) <addr> <value ...> (number of list elements)
        mcuNet.Chip_WBLK(1, [10, 11, 12, 13, 14, 15, 16, 17, 18])

    if 0 :
        #test via the Matlab wrapper:
        NetworkInit()
        #write an array with 12bit values to memory
        #provide 384bits = 32 values for one row
        vals = [0x321, 0x654, 0x987, 0xCBA, 0xFED, 0x210, 0x543, 0x876, 0xBA9, 0xEDC, 0x10F, 0x432, 0x765, 0xA98, 0xDCB, 0x0FE,
                0x321, 0x654, 0x987, 0xCBA, 0xFED, 0x210, 0x543, 0x876, 0xBA9, 0xEDC, 0x10F, 0x432, 0x765, 0xA98, 0xDCB, 0x0FE]
        print("--- write_memory_12bitValues:")
        print("--- values:")
        for i in vals : print("{} ".format(hex(i)), end = '')
        print()
        write_memory_12bitValues(0, vals)
        NetworkClose()

    if 0 :
        #test via the Matlab wrapper:
        NetworkInit()
        #read 12bit values from memory and return as integer array
        #read one row as: 384bits = 32 values
        print("--- read_memory_12bitValues:")
        vals = read_memory_12bitValues(0, 32)
        NetworkClose()

    #interactive command shell (like TELNET until 'q' entered as command):
    if 1 :
        ##mcuNet.TextCommand("help")
        ##mcuNet.TextCommandBinary("help")
        mcuNet.TestLargeTCPCommand()
        #enter endless loop until 'q'
        mcuNet.CommandShell()

if __name__ == '__main__':
    Main()

