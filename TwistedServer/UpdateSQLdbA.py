#!/usr/bin/env python

#This file updates the variables controlling the geyser to the SQL server,
#and thus provides a mechanism for controlling the state machin and
#understanding the process from a web interface.

#THIS FILE IS ASYNC! DO NOT USE BLOCKING CALLS like time.sleep(t)

# Create connection... 
#db = dbmodule.connect('mydb', 'andrew', 'password') 
# ...which blocks for an unknown amount of time 
 
# Create a cursor 
#cursor = db.cursor() 
 
# Do a query... 
#resultset = cursor.query('SELECT * FROM table WHERE ...') 
# ...which could take a long time, perhaps even minutes.

#So, we need to use twisted.enterprise.adbapi where worker threads fetch the
#info while we get a defer


# Using the "mysqldb" module, create a ConnectionPool.
from twisted.enterprise import adbapi
from twisted.internet import defer
dbpool = adbapi.ConnectionPool("MySQLdb", 'localhost', 'AutoGeyser', 'geyserData')


#This routine is to record data to the MySQL database. This class provides
#the methods and memory for such an operation. 
class DataRecorder():
    
    #allocate memory for the target temperature for present run
    #a the run number. In ase, a runn switched, methods provide for a re-init
    #of the runs
    def __init__(self):
        self.runNumber = 0
        self.setTemperature = [0]*6
    
    #Routine to get the previous run number and
    #then create a new table for a new run, and set the present run number
    #use this to reset the run number / start new run
    def makeNewRun(self, targetTemperature):
        setInitial = defer.Deferred()
        self.setTemperature = float(targetTemperature)
        setInitial = self.GetInitialRunNumber()
        setInitial.addCallback(self.SetRunNumber, SQLMode=True)
        setInitial.addCallback(self.MakeTable)
        setInitial.addCallback(self.UpdateRunNumber)
        setInitial.addErrback(self.PrintDebug)
        return setInitial
    
    #Function to get the last run number
    def GetInitialRunNumber(self):
        RunNumber_qry = "SELECT RunNumber FROM AutoGeyser.RunHistory ORDER BY RunNumber DESC LIMIT 1"
        return dbpool.runQuery(RunNumber_qry)
    
    #function to generate new run number, given the last one
    #although trivial in our case, you can change it to make fancy run numbers!
    def UpdateRunNumber(self, runNumber):
        RunNumberUpdate_qry = "INSERT INTO AutoGeyser.RunHistory(`RunNumber` ,`DateTime`, `TargetTemperature`)VALUES (NULL , NULL, %f);" %self.setTemperature
        dbpool.runOperation(RunNumberUpdate_qry)
        return self.runNumber
        
        
        
    #CAUTION!! YOU WILL OVERRIDE SHIT IF NOT CAREFUL!!#
    #Override the run number. DO NOT USE DIRECTLY! #
    def SetRunNumber(self, number, SQLMode = False):
        if SQLMode:
            number = number[0][0]
        self.runNumber = number+1
    
    
    #now, we code a program to gcreate a new data storage table
    #with the present run number
    def MakeTable(self, runNumber):
        RunRec_Qry = """CREATE TABLE IF NOT EXISTS RunHistoryData.%d (
                        id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                        temperature FLOAT,
                        pressure FLOAT,
                        cur_timestamp TIMESTAMP DEFAULT NOW() );""" %self.runNumber
        result = dbpool.runOperation(RunRec_Qry)
        #return None
    
    #Funtion to update the datapoint we are working on.
    def UpdateDatapoint(self, temperature, pressure):
        Update_datapoint_qry = "INSERT INTO RunHistoryData.%d(`id`, `temperature`, `pressure`, cur_timestamp) VALUES (NULL, %f, %f, NULL)" %(self.runNumber, temperature, pressure)
        dbpool.runOperation(Update_datapoint_qry)
        return None
    
    #debugger-print
    def PrintDebug(self, value):
        print "Debug: "+ repr(value)




    
#function to update the variables
def _AUpdateSQL(txn, Temperature, Pressure, SystemState = "n"):
    
    Temperature_qry = "UPDATE AutoGeyser.PresentVectors SET tempValue=%.2f WHERE PresentVectors.IDX =1;" %Temperature
    SystemState_qry = "UPDATE AutoGeyser.PresentVectors SET state=\"%s\" WHERE PresentVectors.IDX =1;" %SystemState
    Pressure_qry = "UPDATE AutoGeyser.PresentVectors SET Pressure=\"%s\" WHERE PresentVectors.IDX =1;" %Pressure
    
    Temperature_record_qry = "INSERT INTO `AutoGeyser`.`_GeyserRunTemperature` (`IDX` ,`Temperature`)VALUES (NULL , %.2f);" %Temperature
    Pressure_record_qry = "INSERT INTO `AutoGeyser`.`_GeyserPressurePSI` (`IDX` ,`Pressure`)VALUES (NULL , %.3f);" %Pressure
    
    txn.execute(Temperature_qry)
    txn.execute(SystemState_qry)
    txn.execute(Pressure_qry)
    
    txn.execute(Temperature_record_qry)
    txn.execute(Pressure_record_qry)
    
    return None

def _AGetInterrupt(txn):
    Temperature_Interrupt_qry = "SELECT ProcessValue FROM AutoGeyser.TemperatureControl WHERE TemperatureControl.IDX =1;"
    txn.execute(Temperature_Interrupt_qry)
    result = txn.fetchall()
    if result:
        return result[0][0]
    else:
        return None

def _OGetInterrupt(txn):
    ONOFF_Interrupt_qry = "SELECT isRunning FROM AutoGeyser.TemperatureControl WHERE TemperatureControl.IDX =1;"
    txn.execute(ONOFF_Interrupt_qry)
    result = txn.fetchall()
    if result:
        return result[0][0]
    else:
        return None

def OGetUserInterrupt():
    return dbpool.runInteraction(_OGetInterrupt)

def CleanTables():
    statement_temp = "TRUNCATE TABLE AutoGeyser.`_GeyserRunTemperature` "
    statement_pressure = "TRUNCATE TABLE AutoGeyser.`_GeyserPressurePSI` "
    dbpool.runOperation(statement_temp)
    dbpool.runOperation(statement_pressure)
    return None

def AUpdateSQLPool(Temperature, Pressure, SystemState = "n"):
    return dbpool.runInteraction(_AUpdateSQL, Temperature, Pressure, SystemState = "n")

def AGetUserInterrupt():
    return dbpool.runInteraction(_AGetInterrupt)
    #Run with
    #AGetUserInterrupt().addCallback(callback)

