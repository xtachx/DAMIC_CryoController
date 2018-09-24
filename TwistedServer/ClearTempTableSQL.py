#!/usr/bin/env python

#we need the mysqldb module for this
import MySQLdb as mdb

def ClearTempTableSQL():
    db = mdb.connect('localhost', 'AutoGeyser', 'spaceball-geyser', 'AutoGeyser');
    #set cursor
    cur = db.cursor()
    #execute the update statement
    #update temp
    statement = "TRUNCATE TABLE `_GeyserRunTemperature` "
    cur.execute(statement)
    db.commit()
    db.close()
    
ClearTempTableSQL()