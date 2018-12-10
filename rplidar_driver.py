#! /usr/bin/python3

## Author: Ryan Brazeal
## Email: ryan.brazeal@ufl.edu
## Date Started: Nov 24th, 2018
## Description: Python3 based driver for SlamTec RPLidar 

## Notes:
##
##    Nov.28th, 2018 - Received an email reply from SlamTec support and apparently new protocol documentation is currently being written to illustrate the new A3 scan rates
##
##    Dec.2nd, 2018 - Standard Scan Rate (~4 KHz) is working properly
##
##    Dec.10th, 2018 - Express Scan Rate (~8 KHz) is working properly
##                     Due to SlamTec protocol document not being updated for A3 device (and I don't feel like reverse engineering the C++ code), no other scan rates are currently working

## RPLidar Viewer Instructions:
##
##    Mouse: 
##        Left Click and Hold - moves (translates) the scan data on the screen
##        Right Click and Hold -  rotates the scan data on the screen (display purposes only, i.e., recorded scan data is unaffected)
##        Mouse Wheel - forward to zoom in, backwards to zoom out
##        Double Left Click - re-center the scan data
##        Double Right Click - re-orientate (rotate) the scan data
##        Double Middle Click - reset the zoom level
##
##    Keyboard:
##        r key    - toogles the faint ray traces on/off within the viewer
##        [/{ key - increases point size
##        ]/} key - decreases point size
##        =/+ key - zooms in
##        -/_ key - zooms out
##        Up/Down/Left/Right keys - moves the scan data in the respective direction
##        ,/< key    - rotates the scan data counter-clockwise
##        ./> key - rotates the scan data clockwise
##        c key    - re-center the scan data
##        o key    - re-orientate (rotate) the scan data
##        z key    - reset the zoom level
##        i key    - re-center and re-orientate and reset the zoom (initializes the default viewer settings)
##        Esc key    - Ends the scanning session and closes the viewer


# Standard Imports
import os
import time
import math
import binascii

#Non-standard Imports
import serial                    #install pyserial
import pygame                    #install pygame

#lidar sensor device class
class RPLidar:
    
    #default constructor
    def __init__(self, port, baudrate, timeout, screenSize, drawRaysDefault, DEBUG):
        #serial port parameters
        self.COM = serial.Serial()
        self.COM.setDTR(False)    #required flag must be set, didn't find mention of this in the documentation
        self.COM.baudrate = baudrate
        self.COM.port = port
        self.COM.timeout = timeout

        #sensor/scanning parameters
        self.pulseRateStandard = 0
        self.pulseRateExpress = 0
        self.rotationRate = 0
        self.new_points = []
        self.zeroPts = 0
        self.startTime = 0
        self.scanCount = 0
        self.totalNumPts = 0
        self.debugMode = DEBUG
                
        #pygame parameters        
        self.screen = None
        self.screenSize = screenSize
        self.drawScale = 0.08 * screenSize
        self.centerX = 0
        self.centerY = 0
        self.pointSize = 2
        self.myfont = None
        self.drawRays = drawRaysDefault
        self.drawRaysDefault = drawRaysDefault
        self.aziCorr = 0.0
        self.csXColour = [255,0,0]
        self.csYColour = [0,255,0]
        self.rayColour = [40,40,40]
        self.pointColour = [0,0,255]
        self.sensorColour = [0,0,0]
        self.bgColour = [20,20,20]
        self.textColour = [255,255,255]
        self.leftButton = False
        self.rightButton = False
        self.mouseClickPos = None
        self.rightClickAzi = 0
        self.lastLeftClickTime = 0
        self.lastRightClickTime = 0
        self.lastMidClickTime = 0
        self.doubleClickTime = 0.25
        print("")
        
    #command to establish a connection between the RPLidar sensor and the computer
    def connect(self):
        #connect to serial port
        try:
            self.COM.close()
            time.sleep(0.1)
        except:
            pass
        self.COM.open()
        time.sleep(0.1)
        
        #setup default pygame environment
        os.environ["SDL_VIDEO_CENTERED"] = "1"
        #os.environ["SDL_VIDEO_WINDOW_POS"] = "%d,%d" % (20,20)
        pygame.init()
        
        #self.myfont = pygame.font.Font('/usr/share/fonts/truetype/freefont/FreeSans.ttf', 14)  # LINUX
        #self.myfont = pygame.font.Font('/System/Library/Fonts/LucidaGrande.ttc', 13)  # MAC
        self.myfont = pygame.font.Font('C:\\Windows\\Fonts\Arial.ttf', 13)   # WINDOWS
        
        pygame.display.set_caption("RPLidar Viewer")
        self.screen = pygame.display.set_mode([self.screenSize,self.screenSize])
        self.screen.fill(self.bgColour)
        self.centerX = int(self.screenSize / 2)
        self.centerY = int(self.screenSize / 2)
        pygame.display.update()

    #command to disconnect the RPLidar sensor from the computer
    def disconnect(self):
        self.COM.close()
        pygame.quit()

    #very ugly checksum generator for '\x00\x11\x22... string format', adds the checksum byte to the end of the input string
    def add_checksum(self, the_bytes):
        checksum = 0
        newCommand = ''
        
        for el in the_bytes:
            checksum ^= ord(el)
            indiv = hex(ord(el))[1:]
            if len(indiv) == 2:
                indiv = indiv[0:1] + "0" + indiv[1:]
            newCommand += '\\' + indiv

        temp = hex(checksum)[1:]
        if len(temp) == 2:
            temp = temp[0:1] + "0" + temp[1:]
        newCommand += '\\' + temp
        temp2 = str.encode(newCommand)
        newCommand = temp2.decode('unicode-escape').encode('ISO-8859-1')
        return(newCommand)

    #main pygame display/draw/update function
    def drawPoints(self):
        #found it more efficient to simply clear the entire display before redrawing features
        self.screen.fill(self.bgColour)

        #new_points list contains only the points collected from the most recent scan (i.e., single rotation of the sensor)
        for i in range(0,len(self.new_points)):
            X = int(self.new_points[i][0] * self.drawScale) + self.centerX
            Y = int(self.new_points[i][1] * self.drawScale) + self.centerY
            currentPos = (X,Y,self.pointSize,self.pointSize)
            #if the user wants to add faint ray traces to the screen
            if self.drawRays:
                pygame.draw.line(self.screen, self.rayColour, (X,Y), (self.centerX,self.centerY))
            pygame.draw.rect(self.screen, self.pointColour, currentPos, 0)
        
        #adds screen text objects to the upper right (dynamic scanning parameters) and lower right (instructions) corners of the screen
        textsurface = self.myfont.render("Rotation Rate:", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (5,5,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(5,5))
        textsurface = self.myfont.render(str(self.rotationRate) + " Hz", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (105,5,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(105,5))
        
        textsurface = self.myfont.render("Points in Scan:", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (5,20,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(5,20))
        textsurface = self.myfont.render(str(len(self.new_points)) + " / " + str(self.zeroPts), False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (105,20,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(105,20))
        
        textsurface = self.myfont.render("Pulses / sec:", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (5,35,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(5,35))
        textsurface = self.myfont.render(str(int((len(self.new_points) + self.zeroPts) * self.rotationRate)), False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (105,35,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(105,35))
        
        textsurface = self.myfont.render("Duration:", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (5,50,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(5,50))
        textsurface = self.myfont.render(str(round(time.time() - self.startTime,1)) + " sec", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (105,50,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(105,50))
        
        textsurface = self.myfont.render("Total Scans:", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (5,65,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(5,65))
        textsurface = self.myfont.render(str(self.scanCount), False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (105,65,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(105,65))
        
        textsurface = self.myfont.render("Total Points:", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (5,80,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(5,80))
        textsurface = self.myfont.render(str(self.totalNumPts), False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (105,80,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(105,80))
        
        textsurface = self.myfont.render("Controls:  Mouse  +  -  [  ]  <  >  up  down  left  right  r  c  z  o  i  Esc", False, self.textColour)
        pygame.draw.rect(self.screen, self.bgColour, (5,self.screenSize - 25,textsurface.get_width()+20,textsurface.get_height()), 0)
        self.screen.blit(textsurface,(5,self.screenSize - 25))

        #draws the sensor object and coordinate system axes to the screen 
        pygame.draw.circle(self.screen, self.sensorColour, [self.centerX, self.centerY], 7, 0)
        pygame.draw.circle(self.screen, [255,255,255], [self.centerX, self.centerY], 7, 1)
        
        #coordinate system axes that don't rotate with the self.aziCorr parameters
        #line1 = pygame.draw.line(self.screen, self.csXColour, (self.centerX,self.centerY), (self.screenSize,self.centerY))
        #line2 = pygame.draw.line(self.screen, self.csYColour, (self.centerX,self.centerY), (self.centerX,0))
        
        #coordinate system axes that do rotate with the self.aziCorr parameters
        csAngle = self.aziCorr * math.pi / 180.0
        x1 = 0.0 
        y1 = 0.0
        x2 = 0.0 
        y2 = 0.0
        if int(self.aziCorr) == 0 or int(self.aziCorr) == 360:
            x1 = self.screenSize
            y1 = self.centerY
            x2 = self.centerX
            y2 = 0
        elif int(self.aziCorr) == 90:
            x1 = self.centerX 
            y1 = 0
            x2 = 0
            y2 = self.centerY
        elif int(self.aziCorr) == 180:
            x1 = 0
            y1 = self.centerY
            x2 = self.centerX 
            y2 = self.screenSize
        elif int(self.aziCorr) == 270:
            x1 = self.centerX
            y1 = self.screenSize
            x2 = self.screenSize 
            y2 = self.centerY
        elif self.aziCorr > 0.0 and self.aziCorr < 90.0:
            x1 = self.screenSize
            y1 = self.centerY - math.tan(csAngle) * (self.screenSize - self.centerX)
            x2 = self.centerX - math.tan(csAngle) * self.centerY
            y2 = 0
        elif self.aziCorr > 90.0 and self.aziCorr < 180.0:
            csAngle -= (1.5 * math.pi)
            x1 = self.centerX - math.tan(csAngle) * self.centerY
            y1 = 0
            x2 = 0
            y2 = self.centerY + math.tan(csAngle) * self.centerX
        elif self.aziCorr > 180.0 and self.aziCorr < 270:
            csAngle -= math.pi
            x1 = 0
            y1 = self.centerY + math.tan(csAngle) * self.centerX
            x2 = self.centerX + math.tan(csAngle) * (self.screenSize - self.centerY)
            y2 = self.screenSize
        elif self.aziCorr > 270 and self.aziCorr < 360:
            csAngle -= (0.5 * math.pi)
            x1 = self.centerX + math.tan(csAngle) * (self.screenSize - self.centerY)
            y1 = self.screenSize
            x2 = self.screenSize
            y2 = self.centerY - math.tan(csAngle) * (self.screenSize - self.centerX)
            
        pygame.draw.line(self.screen, self.csXColour, (self.centerX,self.centerY), (int(x1),int(y1)))
        pygame.draw.line(self.screen, self.csYColour, (self.centerX,self.centerY), (int(x2),int(y2)))
        
        #update the viewer and then reset the necessary parameters
        pygame.display.update()
        self.new_points = []
        self.zeroPts = 0
        
    def getControlInputs(self, stopper):
        funcStopper = stopper
        for events in pygame.event.get():
                    if (events.type == pygame.QUIT):
                        funcStopper = True
                    elif (events.type == pygame.KEYDOWN):
                        if (events.key == pygame.K_ESCAPE):
                            funcStopper = True
                        elif (events.key == pygame.K_RIGHTBRACKET):
                            self.pointSize += 1
                            if self.pointSize > 5:
                                self.pointSize = 5
                        elif (events.key == pygame.K_LEFTBRACKET):
                            self.pointSize -= 1
                            if self.pointSize < 1:
                                self.pointSize = 1
                        elif (events.key == pygame.K_EQUALS):
                            self.drawScale *= 1.1
                            if self.drawScale > 500:
                                self.drawScale = 500
                        elif (events.key == pygame.K_MINUS):
                            self.drawScale *= 0.9
                            if self.drawScale < 10:
                                self.drawScale = 10
                        elif (events.key == pygame.K_DOWN):
                            self.centerY += 20
                            if self.centerY > self.screenSize * 1.25:
                                self.centerY = int(self.screenSize * 1.25)
                        elif (events.key == pygame.K_UP):
                            self.centerY -= 20
                            if self.centerY < -0.25 * self.screenSize:
                                self.centerY = int(-0.25 * self.screenSize)    
                        elif (events.key == pygame.K_RIGHT):
                            self.centerX += 20
                            if self.centerX > self.screenSize * 1.25:
                                self.centerX = int(self.screenSize * 1.25)
                        elif (events.key == pygame.K_LEFT):
                            self.centerX -= 20
                            if self.centerX < -0.25 * self.screenSize:
                                self.centerX = (-0.25 * self.screenSize)
                        elif (events.key == pygame.K_i):
                            self.centerX = int(self.screenSize / 2)
                            self.centerY = int(self.screenSize / 2)
                            self.drawScale = 0.08 * self.screenSize
                            self.pointSize = 2
                            self.aziCorr = 0.0
                            self.drawRays = self.drawRaysDefault
                        elif (events.key == pygame.K_PERIOD):
                            self.aziCorr -= 2
                            if self.aziCorr < 0:
                                self.aziCorr += 360.0
                        elif (events.key == pygame.K_COMMA):
                            self.aziCorr += 2
                            if self.aziCorr > 360:
                                self.aziCorr -= 360.0
                        elif (events.key == pygame.K_z):
                            self.drawScale = 0.08 * self.screenSize
                        elif (events.key == pygame.K_r):
                            self.drawRays = not self.drawRays
                        elif (events.key == pygame.K_c):
                            self.centerX = int(self.screenSize / 2)
                            self.centerY = int(self.screenSize / 2)
                        elif (events.key == pygame.K_o):
                            self.aziCorr = 0.0
                    elif (events.type == pygame.MOUSEBUTTONDOWN):
                        if (events.button == 1):
                            self.leftButton = True
                            self.mouseClickPos = pygame.mouse.get_pos()
                        elif (events.button == 3):
                            self.rightButton = True
                            self.mouseClickPos = pygame.mouse.get_pos()
                            refAzi = math.atan2((self.mouseClickPos[0] - self.centerX),(self.centerY - self.mouseClickPos[1]))
                            if refAzi < 0:
                                refAzi += (2.0 * math.pi)
                            self.rightClickAzi = refAzi
                        elif (events.button == 4):
                            self.drawScale *= 1.1
                            if self.drawScale > 500:
                                self.drawScale = 500
                        elif (events.button == 5):
                            self.drawScale *= 0.9
                            if self.drawScale < 10:
                                self.drawScale = 10
                    elif (events.type == pygame.MOUSEBUTTONUP):
                        nowTime = time.time()
                        if (events.button == 1):
                            self.leftButton = False
                            if nowTime - self.lastLeftClickTime < self.doubleClickTime:
                                self.centerX = int(self.screenSize / 2)
                                self.centerY = int(self.screenSize / 2)
                            self.lastLeftClickTime = nowTime
                        elif (events.button == 2):
                            if nowTime - self.lastMidClickTime < self.doubleClickTime:
                                self.drawScale = 0.08 * self.screenSize
                            self.lastMidClickTime = nowTime
                        elif (events.button == 3):
                            self.rightButton = False
                            if nowTime - self.lastRightClickTime < self.doubleClickTime:
                                self.aziCorr = 0.0
                            self.lastRightClickTime = nowTime
                    elif (events.type == pygame.MOUSEMOTION):
                        if (self.leftButton):
                            currentMousePos = pygame.mouse.get_pos()
                            relX = currentMousePos[0] - self.mouseClickPos[0]
                            relY = currentMousePos[1] - self.mouseClickPos[1]
                            self.centerX += relX
                            self.centerY += relY
                            self.mouseClickPos = (currentMousePos[0], currentMousePos[1])
                        elif (self.rightButton):
                            currentMousePos = pygame.mouse.get_pos()
                            currentAzi = math.atan2((currentMousePos[0] - self.centerX),(self.centerY - currentMousePos[1]))
                            deltaAzi = (currentAzi - self.rightClickAzi) * (180.0 / math.pi)
                            self.aziCorr -= deltaAzi
                            if self.aziCorr < 0:
                                self.aziCorr += 360.0
                            elif self.aziCorr > 360:
                                self.aziCorr -= 360.0
                            self.rightClickAzi = currentAzi
        
        return funcStopper
        
    #query command to get the device information parameters and display them to the terminal window
    def get_device_info(self):
        #self.COM.reset_input_buffer()
        commandBytes = '\xA5\x50'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        time.sleep(0.01)
        inputBuffer = self.COM.read(self.COM.inWaiting())
        inputDataAscii = binascii.hexlify(inputBuffer).decode('ascii')
        inputData = binascii.hexlify(inputBuffer)
        inputBytes = []
        for i in range(0, len(inputData), 2):
            inputBytes.append(inputData[i:i+2])

        read_byte8 = binascii.unhexlify(inputBytes[7])
        read_byte9 = binascii.unhexlify(inputBytes[8])
        read_byte10 = binascii.unhexlify(inputBytes[9])
        read_byte11 = binascii.unhexlify(inputBytes[10])

        model = int.from_bytes(read_byte8, byteorder='little')
        firmware_minor = int.from_bytes(read_byte9, byteorder='little')
        firmware_major = int.from_bytes(read_byte10, byteorder='little')
        hardware = int.from_bytes(read_byte11, byteorder='little')
        serialNum = inputDataAscii[22:]
        
        print("Get Device Info Response: ")
        print("   Model: " + str(model))
        print("   Firmware: " + str(firmware_major) + "." + str(firmware_minor))
        print("   Hardware: " + str(hardware))
        print("   Serial: " + serialNum)
        print("")

    #query command to get the device health parameters and display them to the terminal window
    def get_device_health(self):
        #self.COM.reset_input_buffer()
        commandBytes = '\xA5\x52'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        time.sleep(0.01)
        inputBuffer = self.COM.read(self.COM.inWaiting())
        #inputDataAscii = binascii.hexlify(inputBuffer).decode('ascii')     #DEBUGGING PURPOSES
        inputData = binascii.hexlify(inputBuffer)
        inputBytes = []
        for i in range(0, len(inputData), 2):
            inputBytes.append(inputData[i:i+2])

        read_byte8 = binascii.unhexlify(inputBytes[7])
        read_byte9 = binascii.unhexlify(inputBytes[8])
        read_byte10 = binascii.unhexlify(inputBytes[9])

        status = int.from_bytes(read_byte8, byteorder='little')
        error_code = int.from_bytes((read_byte9 + read_byte10), byteorder='little')
        
        print("Get Health Status Response: ")
        print("   Status: " + str(status))
        print("   Error Code: " + str(error_code))
        print("")

    #query command to get the device's default saaple rates and display them to the terminal window 
    def get_sample_rate(self):
        #self.COM.reset_input_buffer()
        commandBytes = '\xA5\x59'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        time.sleep(0.01)
        inputBuffer = self.COM.read(self.COM.inWaiting())
        #inputDataAscii = binascii.hexlify(inputBuffer).decode('ascii')     #DEBUGGING PURPOSES
        inputData = binascii.hexlify(inputBuffer)
        inputBytes = []
        for i in range(0, len(inputData), 2):
            inputBytes.append(inputData[i:i+2])

        read_byte8 = binascii.unhexlify(inputBytes[7])
        read_byte9 = binascii.unhexlify(inputBytes[8])
        read_byte10 = binascii.unhexlify(inputBytes[9])
        read_byte11 = binascii.unhexlify(inputBytes[10])

        Tstandard = int.from_bytes((read_byte8 + read_byte9), byteorder='little')
        Texpress = int.from_bytes((read_byte10 + read_byte11), byteorder='little')
        PtsPerSecStandard = 1.0 / (float(Tstandard) / 1000000.0)
        PtsPerSecExpress = 1.0 / (float(Texpress) / 1000000.0)

        self.pulseRateStandard = Tstandard
        self.pulseRateExpress = Texpress
        
        print("Get Sample Rate Response: ")
        print("   Standard Scan Sample Rate: " + str(Tstandard) + " us (" + str(int(PtsPerSecStandard)) + " pts/sec)")
        print("    Express Scan Sample Rate: " + str(Texpress) + " us (" + str(int(PtsPerSecExpress)) + " pts/sec)")
        print("")

    # command to start a Standard Scan
    #     - an ASCII file named: "points.csv" will be created in the same location as this .py file and will contain ALL collected points
    #     - if debug Mode is True, a binary file named: "data.bin" will be created in the same location and will contain the standard/express scan observation data 
    def start_standard_scan(self):
        self.startTime = time.time()
        self.lastLeftClickTime = self.startTime
        self.lastRightClickTime = self.startTime
        self.lastMidClickTime = self.startTime

        pwd = os.path.dirname(os.path.realpath(__file__))
        #txtOutput = open(pwd + "/points.csv", 'w')     # LINUX or MAC
        txtOutput = open(pwd + "\\points.csv", 'w')     # WINDOWS
        txtOutput.write("//Scan,X,Y,Z,Intensity\n")     #header line, works well in CloudCompare (www.cloudcompare.org)
        if self.debugMode:
            #dataOutput = open(pwd + "/data.bin" , 'bw')     # LINUX or MAC
            dataOutput = open(pwd + "\\data.bin" , 'bw')    # WINDOWS
        commandBytes = '\xA5\x20'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        time.sleep(0.01)
        
        inputBuffer = self.COM.read(self.COM.inWaiting())
        inputData = bytearray(inputBuffer)

        start1 = inputData[0]
        start2 = inputData[1]
        packetsize = inputData[2]
        midpackets = (inputData[3] << 4) + (inputData[4] << 2) + (inputData[5])
        ending = inputData[6]

        if start1 == 165 and start2 == 90 and packetsize == 5 and midpackets == 64 and ending == 129:

            print("Scan Started (packet size: " + str(packetsize) + " bytes)")
            print("")

            stopper = False
            byteNum = 0
            quality = 0
            angleP1 = 0
            angleP2 = 0
            angle = 0
            distP1 = 0
            distP2 = 0
            dist = 0
            oldTime = time.time()
            newTime = oldTime

            while not stopper:
                byteCount = 0
                bufferDataChunk = self.COM.read(self.COM.inWaiting())
                chunkLength = len(bufferDataChunk)
                if chunkLength > 0:
                    if self.debugMode:
                        dataOutput.write(bufferDataChunk)
                    bufferData = bytearray(bufferDataChunk)
                    
                    for i in range(0,chunkLength):
                        
                        if byteNum == 0:
                            byteNum = 1
                            val = (bufferData[i]) & 3
                            quality = (bufferData[i] >> 2)
                            if val == 1:
                                self.scanCount += 1
                                newTime = time.time()
                                
                                self.rotationRate = round((1.0 / (newTime - oldTime)),1)
                                oldTime = newTime
                                self.drawPoints()
                                
                        elif byteNum == 1:
                            byteNum = 2
                            angleP2 = (bufferData[i] >> 1)
                        elif byteNum == 2:
                            byteNum = 3
                            angleP1 = (bufferData[i] << 7)
                            angle = (float(angleP1 + angleP2) / 64.0) * (math.pi / 180.0)   #angle in radians
                        elif byteNum == 3:
                            byteNum = 4
                            distP2 = (bufferData[i])
                        else:  # byteNum == 4
                            byteNum = 0
                            distP1 = (bufferData[i] << 8)
                            dist = float(distP1 + distP2) / 4000.0  #dist in meters
                            if dist > 0:
                                deg90 = math.pi / 2.0
                                X = round(math.sin(angle + deg90) * dist,3)
                                Y = round(math.cos(angle + deg90) * dist,3)
                                Xv = math.cos(angle - (self.aziCorr * math.pi / 180.0)) * dist
                                Yv = math.sin(angle - (self.aziCorr * math.pi / 180.0)) * dist
                                self.totalNumPts += 1
                                self.new_points.append([Xv,Yv])
                                txtOutput.write(str(self.scanCount) + "," + str(X) + "," + str(Y) + ",0," + str(quality) + "\n")
                            else:
                                self.zeroPts += 1
                        byteCount += 1
                              
                stopper = self.getControlInputs(stopper)
                                                            
        pygame.quit()
        txtOutput.close()
        if self.debugMode:
            dataOutput.close()

    def calcAndAddPoints(self,dist1,dist2,delta1,delta2,previousAngle,refAngle,txtOutput):
        angleDiff = float(refAngle - previousAngle)
        if (angleDiff) < 0:
            angleDiff += (math.pi * 2.0)
        
        deg90 = math.pi / 2.0
        
        pointCount = -1
        for i in range(0, 16):
            pointCount += 1
            if dist1[i] > 0:
                ptAngle1 = previousAngle + ((angleDiff / 32.0) * float(pointCount)) - delta1[i]
                X = round(math.sin(ptAngle1 + deg90) * dist1[i],3)
                Y = round(math.cos(ptAngle1 + deg90) * dist1[i],3)
                Xv = math.cos(ptAngle1 - (self.aziCorr * math.pi / 180.0)) * dist1[i]
                Yv = math.sin(ptAngle1 - (self.aziCorr * math.pi / 180.0)) * dist1[i]
                self.new_points.append([Xv,Yv])
                self.totalNumPts += 1
                txtOutput.write(str(self.scanCount) + "," + str(X) + "," + str(Y) + ",0\n")
            else:
                self.zeroPts += 1
            pointCount += 1
            if dist2[i] > 0:
                ptAngle2 = previousAngle + ((angleDiff / 32.0) * float(pointCount)) - delta2[i]
                X = round(math.sin(ptAngle2 + deg90) * dist2[i],3)
                Y = round(math.cos(ptAngle2 + deg90) * dist2[i],3)
                Xv = math.cos(ptAngle2 - (self.aziCorr * math.pi / 180.0)) * dist2[i]
                Yv = math.sin(ptAngle2 - (self.aziCorr * math.pi / 180.0)) * dist2[i]
                self.new_points.append([Xv,Yv])
                self.totalNumPts += 1
                txtOutput.write(str(self.scanCount) + "," + str(X) + "," + str(Y) + ",0\n")
            else:
                self.zeroPts += 1

    def readCabin1or3(self, data):
        distP2 = (data >> 2)
        deltaP1 = ((data & 1) << 4)
        deltaSign = -1*((data >> 1) & 1)
        if deltaSign == 0:
            deltaSign = 1
        return distP2, deltaP1, deltaSign
    
    def readCabin2or4(self, data):
        distP1 = (data << 6)   
        return distP1
    
    def readCabin5(self, data):
        delta2P2 = (data >> 4)
        delta1P2 = (data & 15)
        return delta1P2, delta2P2
    
    def calcDist(self, distP1, distP2):
        return (float(distP1 + distP2) / 1000.0)    #distance in meters
        
    def calcDelta(self, deltaP1, deltaP2, deltaSign):
        value = float(deltaSign) * (float(deltaP1 + deltaP2) / 8.0) * (math.pi/180.0)
        return (value)  #delta angle in radians
    
    def start_express_scan(self):
        self.startTime = time.time()
        self.lastLeftClickTime = self.startTime
        self.lastRightClickTime = self.startTime
        self.lastMidClickTime = self.startTime
        pwd = os.path.dirname(os.path.realpath(__file__))
        #txtOutput = open(pwd + "/points.csv", 'w')     # LINUX or MAC
        txtOutput = open(pwd + "\\points.csv", 'w')     # WINDOWS
        txtOutput.write("//Scan,X,Y,Z\n")     #header line, works well in CloudCompare (www.cloudcompare.org)
        if self.debugMode:
            #dataOutput = open(pwd + "/data.bin" , 'bw')    # LINUX or MAC
            dataOutput = open(pwd + "\\data.bin" , 'bw')    # WINDOWS
        commandBytes = '\xA5\x82\x05\x00\x00\x00\x00\x00'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        time.sleep(0.01)
        
        inputBuffer = self.COM.read(self.COM.inWaiting())
        inputData = bytearray(inputBuffer)

        start1 = inputData[0]
        start2 = inputData[1]
        packetsize = inputData[2]
        midpackets = (inputData[3] << 4) + (inputData[4] << 2) + (inputData[5])
        ending = inputData[6]

        if start1 == 165 and start2 == 90 and packetsize == 84 and midpackets == 64 and ending == 130:

            print("Express Scan Started (packet size: " + str(packetsize) + " bytes)")
            print("")

            stopper = False
            byteNum = 0
            refAngleP1 = 0
            refAngleP2 = 0
            refAngle = 0
            delta1P1 = 0
            delta1P2 = 0
            delta1 = []
            deltaSign1 = 1
            delta2P1 = 0
            delta2P2 = 0
            delta2 = []
            deltaSign2 = 1
            dist1P1 = 0
            dist1P2 = 0
            dist1 = []
            dist2P1 = 0
            dist2P2 = 0
            dist2 = []
            oldTime = time.time()
            newTime = oldTime
            previousAngle = 0

            while not stopper:
                byteCount = 0
                bufferDataChunk = self.COM.read(self.COM.inWaiting())
                #inputDataAscii = binascii.hexlify(bufferDataChunk).decode('ascii')     #DEBUGGING PURPOSES

                chunkLength = len(bufferDataChunk)
                if chunkLength > 0:
                    if self.debugMode:
                        dataOutput.write(bufferDataChunk)
                    bufferData = bytearray(bufferDataChunk)
                    
                    while byteCount < chunkLength:
                        #txtOutput.write(str(hex(int(str(bufferData[byteCount])))) + " ")     #DEBUGGING PURPOSES
                        #txtOutput.write(format(int(str(bufferData[byteCount])), '08b'))      #DEBUGGING PURPOSES

                        if byteNum == 0:
                            check1 = (bufferData[byteCount] >> 4)
                            if check1 == 10:
                                byteNum = 1
                        
                        elif byteNum == 1:
                            check2 = (bufferData[byteCount] >> 4)
                            if check2 == 5:
                                byteNum = 2
                            else:
                                byteNum = 0
                        
                        elif byteNum == 2:
                            byteNum = 3
                            refAngleP2 = bufferData[byteCount]
                        
                        elif byteNum == 3:
                            refAngleP1 = ((bufferData[byteCount] & 127) << 8)
                            refAngle = (float(refAngleP1 + refAngleP2) / 64.0) * (math.pi/180.0) #angle in radians

                            if refAngle - previousAngle < 0:
                                self.scanCount += 1
                                newTime = time.time()
                                self.rotationRate = round((1.0 / (newTime - oldTime)),1)
                                oldTime = newTime
                                self.drawPoints()
                                
                            if self.scanCount > 1:
                                self.calcAndAddPoints(dist1,dist2,delta1,delta2,previousAngle,refAngle,txtOutput)
                                dist1 = []
                                dist2 = []
                                delta1 = []
                                delta2 = []
                            byteNum = 4
                            
                        #cabin data
                        elif byteNum == 4:
                            byteNum = 5
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 5:
                            byteNum = 6
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 6:
                            byteNum = 7
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 7:
                            byteNum = 8
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 8:
                            byteNum = 9
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 9:
                            byteNum = 10
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 10:
                            byteNum = 11
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 11:
                            byteNum = 12
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 12:
                            byteNum = 13
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 13:
                            byteNum = 14
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 14:
                            byteNum = 15
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 15:
                            byteNum = 16
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 16:
                            byteNum = 17
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 17:
                            byteNum = 18
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 18:
                            byteNum = 19
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 19:
                            byteNum = 20
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 20:
                            byteNum = 21
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 21:
                            byteNum = 22
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 22:
                            byteNum = 23
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 23:
                            byteNum = 24
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 24:
                            byteNum = 25
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 25:
                            byteNum = 26
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 26:
                            byteNum = 27
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 27:
                            byteNum = 28
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 28:
                            byteNum = 29
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 29:
                            byteNum = 30
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 30:
                            byteNum = 31
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 31:
                            byteNum = 32
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 32:
                            byteNum = 33
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 33:
                            byteNum = 34
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 34:
                            byteNum = 35
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 35:
                            byteNum = 36
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 36:
                            byteNum = 37
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 37:
                            byteNum = 38
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 38:
                            byteNum = 39
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 39:
                            byteNum = 40
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 40:
                            byteNum = 41
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 41:
                            byteNum = 42
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 42:
                            byteNum = 43
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 43:
                            byteNum = 44
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 44:
                            byteNum = 45
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 45:
                            byteNum = 46
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 46:
                            byteNum = 47
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 47:
                            byteNum = 48
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 48:
                            byteNum = 49
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 49:
                            byteNum = 50
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 50:
                            byteNum = 51
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 51:
                            byteNum = 52
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 52:
                            byteNum = 53
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 53:
                            byteNum = 54
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 54:
                            byteNum = 55
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 55:
                            byteNum = 56
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 56:
                            byteNum = 57
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 57:
                            byteNum = 58
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 58:
                            byteNum = 59
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 59:
                            byteNum = 60
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 60:
                            byteNum = 61
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 61:
                            byteNum = 62
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 62:
                            byteNum = 63
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 63:
                            byteNum = 64
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 64:
                            byteNum = 65
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 65:
                            byteNum = 66
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 66:
                            byteNum = 67
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 67:
                            byteNum = 68
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 68:
                            byteNum = 69
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 69:
                            byteNum = 70
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 70:
                            byteNum = 71
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 71:
                            byteNum = 72
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 72:
                            byteNum = 73
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 73:
                            byteNum = 74
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 74:
                            byteNum = 75
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 75:
                            byteNum = 76
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 76:
                            byteNum = 77
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 77:
                            byteNum = 78
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 78:
                            byteNum = 79
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                        elif byteNum == 79:
                            byteNum = 80
                            dist1P2, delta1P1, deltaSign1 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 80:
                            byteNum = 81
                            dist1P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 81:
                            byteNum = 82
                            dist2P2, delta2P1, deltaSign2 = self.readCabin1or3(bufferData[byteCount])
                        elif byteNum == 82:
                            byteNum = 83
                            dist2P1 = self.readCabin2or4(bufferData[byteCount])
                        elif byteNum == 83:
                            byteNum = 0
                            delta1P2, delta2P2 = self.readCabin5(bufferData[byteCount])
                            dist1.append(self.calcDist(dist1P1, dist1P2))
                            dist2.append(self.calcDist(dist2P1, dist2P2))
                            delta1.append(self.calcDelta(delta1P1, delta1P2, deltaSign1))
                            delta2.append(self.calcDelta(delta2P1, delta2P2, deltaSign2))
                            previousAngle = refAngle
                            
                        byteCount += 1
                              
                stopper = self.getControlInputs(stopper)

        pygame.quit()
        txtOutput.close()
        if self.debugMode:
            dataOutput.close()

    # start motor to maximum rotation rate command (~20 Hz)
    def start_motor_max(self):
        commandBytes = '\xA5\xF0\x02\xFF\x03'
        #commandBytes = '\xA5\xF0\x02\x02\x00' #(super slow)
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        print("Motor Started (max speed)")
        print("")
        time.sleep(6.0)

    # start motor to 75% rotation rate command (~15 Hz)
    def start_motor_34(self):
        commandBytes = '\xA5\xF0\x02\x18\x03'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        print("Motor Started (3/4 speed)")
        print("")
        time.sleep(6.0)
    
    # start motor to half rotation rate command (~10 Hz)
    def start_motor_half(self):
        commandBytes = '\xA5\xF0\x02\x3F\x02'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        print("Motor Started (1/2 speed)")
        print("")
        time.sleep(6.0)

    # start motor to 25% rotation rate command (~5 Hz)
    def start_motor_14(self):
        commandBytes = '\xA5\xF0\x02\x4A\x01'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        print("Motor Started (1/4 speed)")
        print("")
        time.sleep(6.0)
    
    # start motor to minimum (useable) rotation rate command (~1.5 Hz)
    def start_motor_slow(self):
        commandBytes = '\xA5\xF0\x02\x80\x00'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        print("Motor Started (slow speed)")
        print("")
        time.sleep(6.0)

    # stop motor command (set to 0% rotation rate, 0 Hz)
    def stop_motor(self):
        commandBytes = '\xA5\xF0\x02\x00\x00'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        print("Motor Stopped")
        print("")

    # stop scan command, stops the transmission of data
    def stop_scan(self):
        commandBytes = '\xA5\x25'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        print("Scan Stopped")
        print("")

    # reset command, should be used for proper communication/functionality checks, currently I'm not doing this
    def reset(self):
        commandBytes = '\xA5\x40'
        commandBytes = self.add_checksum(commandBytes)
        self.COM.write(commandBytes)
        print("System Reset")
        print("")





# Example program using RPLidar class object
if __name__ == "__main__":
    try:
                           #Serial Port  , Baud Rate, Timeout, Screen Size, Draw Rays, Debug Mode                           
        #sensor = RPLidar('/dev/ttyUSB0',    256000,       1,         700,      True,       True)   # LINUX
        #sensor = RPLidar('/dev/cu.SLAB_USBtoUART',    256000,       1,         700,      True,       True)   # MAC
        sensor = RPLidar('COM2',    256000,       1,         700,      True,       True)   # WINDOWS
        sensor.connect()
        sensor.stop_scan()
        sensor.stop_motor()
        
        sensor.get_device_info()
        sensor.get_device_health()
        sensor.get_sample_rate()
        
        #sensor.start_motor_slow()
        #sensor.start_motor_14()
        #sensor.start_motor_half()
        sensor.start_motor_34()
        #sensor.start_motor_max()
        
        #sensor.start_standard_scan()
        sensor.start_express_scan()
        
        sensor.stop_scan()
        sensor.stop_motor()
        sensor.disconnect()
    
    except:
        print(" *** AN UNKNOWN ERROR OCCURRED *** ")
        sensor.stop_scan()
        sensor.stop_motor()
        sensor.disconnect()
        
        
