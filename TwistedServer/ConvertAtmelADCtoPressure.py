#!/usr/bin/env python


def ADC2Pressure(ADCVal):
    ADCVal = float(ADCVal)
    Voltage = ADCVal*5.0 / 1024.0
    #the resistance across which we measure voltage
    Resistance = 296.5 #Ohm
    #Ohm's Law: V=IR
    Current = Voltage / Resistance
    #Since our current range is 4mA-20mA, we have to subtract 4mA
    #to get the current signal
    Offset = 0.004
    CurrentSignal = Current-Offset
    
    #Now, 4mA = 0PSI and 20mA = 100psi.
    #In terms of Isignal, 16mA = 100psi.
    #We measure mA, we need to find x mA = ? PSI.
    #1mA = 100/16 PSI
    Pressure = CurrentSignal * 100 / 0.016
    Pressure_Formatted = str("%.02f" % Pressure)
    
    #Some sanity checks
    
    #CurrentSignal should never be more than 16mA or less than 0 mA
    #if CurrentSignal<0:
    #    print " -- We have negative signal. Time: "+str(time.ctime())+" !\n"
    #if CurrentSignal>0.016:
    #    print " -- We have overvoltage! \n"
    return Pressure_Formatted
    
    