#!/usr/bin/env python

#This file updates the variables controlling the state machine,
#and thus provides a mechanism for controlling the state machine.

#we need the mysqldb module for this
import MySQLdb as mdb
#need this for "sleep"
import time

#The first part will update the database with the present vars
#so that we can post a real-time status on our control scirpt
def Update_DB(processvalue, Pressure, SystemState="n"):
    #connect to db
    db = mdb.connect('localhost', 'AutoGeyser', 'spaceball-geyser', 'AutoGeyser');
    #set cursor
    cur = db.cursor()
    #execute the update statement
    #update temp
    statement = "UPDATE AutoGeyser.PresentVectors SET tempValue=%.2f WHERE PresentVectors.IDX =1;" %processvalue
    cur.execute(statement)
    #update state    
    statement2 = "UPDATE AutoGeyser.PresentVectors SET state=\"%s\" WHERE PresentVectors.IDX =1;" %SystemState
    cur.execute(statement2)
    #update pressure
    statement3 = "UPDATE AutoGeyser.PresentVectors SET Pressure=\"%s\" WHERE PresentVectors.IDX =1;" %Pressure
    cur.execute(statement3)
    #update the temperature table
    statement4 = "INSERT INTO `AutoGeyser`.`_GeyserRunTemperature` (`IDX` ,`Temperature`)VALUES (NULL , %.2f);" %processvalue
    cur.execute(statement4)
    #update pressure table
    statement5 = "INSERT INTO `AutoGeyser`.`_GeyserPressureVoltage` (`IDX` ,`Pressure`)VALUES (NULL , %.3f);" %Voltage
    cur.execute(statement5)
    #commit database
    db.commit()
    #close connection
    db.close()
    
    
    #TRUNCATE TABLE `_GeyserRunTemperature` 

    

#this part gets the the state machine updates with the input from the
#operator.
def get_SM_updates():
    #connect to database
    db = mdb.connect('localhost', 'AutoGeyser', 'spaceball-geyser', 'AutoGeyser');
    #set the cursor
    cur = db.cursor()
    #get the set temperature value from table
    statement = "SELECT ProcessValue FROM AutoGeyser.TemperatureControl WHERE TemperatureControl.IDX =1;"
    cur.execute(statement)
    row = cur.fetchone()
    
    #close db
    db.close()
    #return the new value
    return row[0]

#this is the controller function for the updates. This will call
#Update_DB or Update_SM
def UpdateControl(TempDataInStream, SetTemp, SystemState, PressureVoltage):
    
    while True:
        #Update the database with the present temperature value of the geyser
        SState = SystemStateDictionary(SystemState[0])
        Update_DB(TempDataInStream.value, SState, PressureVoltage.value)
        #store the present PV value in a variable. This helps us to
        #avoid unnececery updates on a shared variable
        present_PV = SetTemp.value
        #get the new PV in database if any, and compare
        new_pv = get_SM_updates()
        if new_pv != present_PV:
            #if they differ, update PV
            SetTemp.value = new_pv
        time.sleep(1)
        
        #TRUNCATE TABLE `_GeyserRunTemperature` 
    
