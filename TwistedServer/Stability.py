#!/usr/bin/env python

#!/usr/bin/env python

#################################################################
#This is the Stability Class of the Geyser Contol               #
#Eventually we may build a web based GUI, but dont count on it! #
#Made by Pitam Mitra for PICASSO detector R&D                   #
#Released under GNU GPL                                         #
#################################################################


import numpy



#The main contol would be the temperature readout.
#The logic is that we will have a 2-step boxcar average,
#a leading average and a trailing average.
#We do the Leading Average - Trailing average, and if this
#falls drastically below zero, we have a problem

class CheckStablity():
    
    def __init__(self, step, interval):
        self.step = step
        self.interval = interval
        self.chunksize = 2*self.step + interval

        self._BeginEvent = 0
        self._EventTimeTracker = 0
        self._cursor = 0
        self._InvestigateFlag = False
        self._SuspectAnomaly = False
        #Pressure stuff
        #self.
    
    
    def DetectEventPressure(self, CurreentPressure):
        pass
    
    def DetectEventAndAnomaly(self,leadAVG, trailAVG):
        leadAVG = float(leadAVG)
        trailAVG = float(trailAVG)
 
        _cursor = leadAVG-trailAVG

        if (_cursor < -1.0) and (self._InvestigateFlag==False) :
            self._EventTimeTracker = 0
            self.AnomalyProbability = 0.1
            self.EventProbability = 0.1
            self._InvestigateFlag = True
            print "Investigation Begin: "
        if self._InvestigateFlag == True:
            self._EventTimeTracker+=1
            if _cursor >= 0.25:
                # "Likely Event! 
                pass
            
            #Suspected Anomaly
            if _cursor <= -3:
                self._SuspectAnomaly = True
                
            #Unstable Geyser
            if (self._EventTimeTracker >= self.chunksize) and self._SuspectAnomaly:
                print "**********Geyser is unstable****************"
                return 1
            
            #If there were some fluctuations, which were not peaks but were detected,
            #reset the control back to normal.
            if (self._EventTimeTracker >= 2*self.chunksize) and not self._SuspectAnomaly:
                self._SuspectAnomaly = False
                self._EventTimeTracker = 0
                self._InvestigateFlag = False
                return 2
            
            #If a peak is detected
            if _cursor >= +1.0:
                self._InvestigateFlag = False
                self._SuspectAnomaly = False
                self._EventTimeTracker = 0
                
                print "Caught an Event!"
                return 0
        
        return 0

