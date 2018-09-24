#!/usr/bin/env python

#This is the kernel for the geyser's state-machine control system
#The kernel's job is to not babysit the analysis and determination of states.
#The kernel connects to all parts of the system / hardware.

#The state machine is like the OS, it DECIDES which state its in and controls
#through the kernel.


from multiprocessing import Process, Queue, Value, Array
import time, sys, os
import ArduinoDaemon
import StateMachineINO as StateMachine
import ClearTempTableSQL

TempDataInStream = Queue()
TempDaemonStopFlag = Queue()

leadQ = Queue()
breakQ = Queue()
trailQ = Queue()

leadAVG = Value('f', 0.0)
trailAVG = Value('f', 0.0)

step = Value('i', 20)
interval = Value('i',2)
Parent_PID = os.getpid()

SystemState = Array('c',1)
SystemState[0] = "s"

SetTemp = Value('d', 0)

PressureVoltage = Value('f', 0.0)



def PopulateQ(TempDataInStream, leadQ, breakQ, trailQ, leadAVG, trailAVG, step, interval):
    
    ##################################################
    #Populate the Queues                             #
    #This part of the code populates the queues.     #
    #We need this before we begin any work!          #
    #Without the queues populated the Moving Average #
    #filter cannot not do its magic!                 #
    ##################################################
    
    #Inform the user that we start with populating the queue, and how long
    #it will take
    print "Populating Queues, getting ready to start..."
    print "This will take "+str(2*step.value+interval.value)+"s with a 1 Hz DAQ"
    i = 0 #this is to track how many points we have populated
    #The while statement makes sure we populate only 2* step + initial points
    while i <= 2*step.value+interval.value-1:
        #Read the Sensor
        ThermData = TempDataInStream.get()
        #put the first "Ste" number of points in trail queue
        if i<step.value:
            trailQ.put(ThermData)
            trailAVG.value+=ThermData
        #the middle ones go to breakQ
        if i>=step.value and i<step.value+interval.value:
            breakQ.put(ThermData)
        #the last ones go to leadQ
        if i>=step.value+interval.value:
            leadQ.put(ThermData)
            leadAVG.value+=ThermData
        #increment i
        i+=1
        #sleep for 1 second, our DAQ is 1Hz!
        time.sleep(1)
        
        sys.stdout.flush()
        sys.stdout.write("\rDaemon starts in: "+str(2*step.value+interval.value-i)+" seconds. Datapoint: "+str(ThermData))
        #print i
        #print ThermData
        #########End Populatate Queues###########
        
    ####Set the Leading and trailing averages#######
    leadAVG.value/=step.value
    trailAVG.value/=step.value
    print "\nFinished populating Queues...\n"
    ################################################
    
def ReadyTemp(TempDataInStream,leadQ, breakQ, trailQ, leadAVG, trailAVG, step, interval):
    time.sleep(1)
    print "Starting temperature memory updater daemon."
    TempDaemonRun = True
    count = 0
    while TempDaemonRun:
        ##############################################
        #if count >=10: print "Putting the stop flag"
        #count+=1
        #if count>=10: TempDaemonRun = False
        #############################################
        
        data_in = TempDataInStream.get()
        
        lead_pop = leadQ.get()
        break_pop = breakQ.get()
        trail_pop = trailQ.get()
        
        #--------------------#
        leadQ.put(data_in)
        breakQ.put(lead_pop)
        trailQ.put(break_pop)
            
        leadAVG.value+=(data_in-lead_pop)/step.value
        trailAVG.value+=(break_pop-trail_pop)/step.value
        
        ##############################################
        
        
        



#########################################

#Process(target=TempDaemonFPOptomux.Read_TC, args=(TempDataInStream, TempDaemonStopFlag, "0004")).start()
Process(target=ArduinoDaemon.ControlArduino, args=(TempDataInStream, TempDaemonStopFlag, PressureVoltage)).start()
ClearTempTableSQL.ClearTempTableSQL()
print "Note: Web Based Monitoring wont start until the population of queue is complete! Be patient!"
PopulateQ(TempDataInStream,leadQ, breakQ, trailQ, leadAVG, trailAVG, step, interval)
Process(target=ReadyTemp, args=(TempDataInStream, leadQ, breakQ, trailQ, leadAVG, trailAVG, step, interval)).start()
Process(target=UpdateVars.UpdateControl, args=(leadAVG, SetTemp, SystemState, PressureVoltage)).start()
Process(target=StateMachine.Governor, args=(SetTemp, step, interval, leadAVG, trailAVG, Parent_PID, SystemState, PressureVoltage)).start()


