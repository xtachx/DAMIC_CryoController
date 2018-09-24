#!/usr/bin/env python
#################################################################
#This is the State Machine of the Geyser Contol Ops             #
#Eventually we may build a web based GUI, but dont count on it! #
#Made by Pitam Mitra for PICASSO detector R&D                   #
#Released under GNU GPL                                         #
#################################################################

#Ok so I have given this a lot of thought about this program. FSMs, or
#Finite state machines are not supposed to run in a "superloop" like we are
#doing at the moment. We should be using Event based triggers ala
#Twisted.internet. So I will attempt to redo the entire thing
#based on twisted.

#we need the reactor from twisted.internet, and the protocol
#factory for our server.
from twisted.internet import reactor, protocol, task, threads, defer

#import time module for sleep functions
import time

#import sys for some extra functions like the status print to CLI etc
import sys

#Arduino Geyser control protocol
import GeyserProto as GP


#numpy module for averaging
import numpy as np

#this is for the SQL updates
import UpdateSQLdbA as AuSQL

#This is for the TUI
from TUI import GeyserTUI as gTUI


#Some HariKari measures in case stuff goes wrong...
#Seriously, this kills the whole Control system!
def killall():    
    reactor.stop()

class GeyserEvent():
    
    def __init__(self):
        #########################
        #-------VARIABLES-------#
        #########################
        #Variables which define the desired
        #parameters of the geyser
        self.setPointCoolingTop = 10.0
        self.setPointCoolingMid = 30.0
        self.setPointCoolingBot = 50.0
        
        #Variables which define a stable geyser
        #####
        self.UnstablePressure = 25.0 #PSI
        self.UnstableTime = 30.0 #sec
        
        
        #Variables to track the geyser state
        self.isStable = True
        self.TemperatureData = [0.0]*12
        self.Pressure = 0.0
        
        #Variables which defines the math routines for stability calculation
        #and stores the memory locations for data being processed
        self.step = 9
        self.interval = 2
        self.LeadAvgPressure = 0.0
        self.TrailAvgPressure = 0.0
        self.updateTimeInterval = 1.0 #secs
        
        #Variables for the data recording function
        RunNumber = 0

        #########################
        #------- Text UI -------#
        #########################
        
        self.TUI = gTUI.AutoGetserTUI()
        self.TUI.start()
        
        ########################################
        #-------Initialization Processes-------#
        ########################################
        
        #Refresh memory, clean SQL tables
        self.Initialize()
        self.dataRecorder = AuSQL.DataRecorder()
        #Set the running status indicator to false unless
        #started explicitly
        self._isRunning = False
        
        
        ################################
        #-------Periodic Updates-------#
        ################################
        runDispatcher = task.LoopingCall(self.ComputeProcess)
        runDispatcher.start(self.updateTimeInterval)
        ####
        procDispatcher = task.LoopingCall(self.UpdateVariables_Running)
        procDispatcher.start(self.updateTimeInterval)
        #
        
        
        #self.writeHalt = False
        
    
    #What to run, when the process starts up for the FIRST TIME
    def Initialize(self):
        #Populate the temperatures and the pressure
        self.UpdateVars()
        AuSQL.CleanTables()
        
        #To start evaluating the stability, we need an array of pressures
        #for the box car averaging
        self.TemperatureData += [float(GP.getcFP())] * self.BoxCarSize()
    
    #Function to refresh the memory with new data from the geyser operation
    def UpdateVars(self):
        for i in xrange(0,11):
            self.TemperatureData[i] = GP.getTemperature(i)
        self.Pressure = GP.getPressure()
    
    #function to refresh the memory ONLY if there is an active run
    def UpdateVariables_Running(self):
        if self._isRunning == True:
            reactor.callWhenRunning(self.UpdateVars)
    
    #Function to update the display and compute Stability
    def ComputeProcess(self):
        #Check for user interrupts first
        #and then handle such interrupts.
        #At the moment, the code is "toothless"
        #i.e. cant change shit
        reactor.callWhenRunning(self.SQLInterrupts)
        if self._isRunning == True:
            #Trigger the Automatic SQL backup
            #NOTE: To check!!
            reactor.callWhenRunning(self.dataRecorder.UpdateDatapoint, self.TemperatureData, self.Pressure)
            
            #Update the TUI display
            self.TUI.updateTemperature(self.TemperatureData)
            self.TUI.updatePressure(self.Pressure)
            self.TUI.updateDisplay()
            
        
    #Make a new run number and change the present run number. To
    #start a new run from an existing run
    def changeRun(self):
        self._isRunning = False
        onSetTable = self.dataRecorder.makeNewRun(self.setPoint)
        onSetTable.addCallback(self.setRunNumberandStart)

    #Code to set a new run number and set the isRunning
    #flag as true
    def setRunNumberandStart(self, RunNumber):
        print "Set new run number success; Run# "+ str(RunNumber)+" is now live!"
        self.RunNumber = RunNumber
        self._isRunning = True
        return None
    
    BoxCarSize = lambda self : 2*self.step + self.interval
    
    def RunStabilityCheck(self):
        StabilityStatus = defer.Deferred()
        StabilityAnalysisResult = self.CheckStablity.DetectEventAndAnomaly(self.LeadAvg, self.TrailAvg)
        StabilityStatus.callback(StabilityAnalysisResult)
        return StabilityStatus
    
    def StabilityAnalysis(self, StabilityResponse):
        if StabilityResponse == 1:
            print "StabilityResponse - Geyser is Unstable!!", repr(StabilityResponse)
    
    def StabilityWrapper(self):
        Status = self.RunStabilityCheck()
        Status.addCallback(self.StabilityAnalysis)
    
    #Code to cycle a run based on an On/OFF instruction
    #Basically, the ON/OFF switch toggle from the web
    #NOTE: To be checked with new DAQ
    def ONOFFCallback(self, new_instruction):
        NewInstruction = bool(new_instruction)
        if NewInstruction != self._isRunning:
            if NewInstruction == False:
                self.setPoint = 0.0
                self.UpdateVars()
            self._isRunning = NewInstruction
            print "Detector status has been toggled. New run started by OGetInterrupt...!"
            if NewInstruction == True:
                self.changeRun()

    #Code to process user interrupts during runtime
    #Like start/stop run and/or run parameter change
    #handle these once every second
    def SQLInterrupts(self):
        AuSQL.AUpdateSQLPool(self.LeadAvg, self.Pressure)
        AuSQL.AGetUserInterrupt().addCallback(self.SetPointInterruptCallback)
        AuSQL.OGetUserInterrupt().addCallback(self.ONOFFCallback)
        
        
GeyserEventDispacher = GeyserEvent()
#reactor.callWhenRunning(GeyserEventDispacher.Initialize())
reactor.run()
