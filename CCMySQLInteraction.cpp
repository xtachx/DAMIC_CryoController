//
//  CCMySQLInteraction.cpp
//  CC_R
//
//  Created by Pitam Mitra on 1/6/19.
//  Copyright © 2019 Pitam Mitra. All rights reserved.
//

/*Function to interact with MySQL*/

#include <mysqlx/xdevapi.h>
#include "MysqlCredentials.hpp"
#include "CryoControlSM.hpp"
#include <iostream>




void CryoControlSM::UpdateVars(DataPacket &_thisInteractionData ){
    
    // Connect to server using a connection URL
    mysqlx::Session DDroneSession("localhost", 33060, DMysqlUser, DMysqlPass);
    mysqlx::Schema DDb = DDroneSession.getSchema("DAMICDrone");
    
    /*First lets get the control parameters*/
    mysqlx::Table CtrlTable = DDb.getTable("ControlParameters");
    mysqlx::RowResult ControlResult = CtrlTable.select("TTemperature", "Kp", "Ki", "Kd", "KpR","KiR", "KdR", "CCPowerState","WatchdogFuse")
    .bind("IDX", 1).execute();
    /*The row with the result*/
    mysqlx::Row CtrlRow = ControlResult.fetchOne();
    
    _thisInteractionData.TTemp = CtrlRow[0];
    _thisInteractionData.kpA = CtrlRow[1];
    _thisInteractionData.kiA = CtrlRow[2];
    _thisInteractionData.kdA = CtrlRow[3];
    _thisInteractionData.kpR = CtrlRow[4];
    _thisInteractionData.kiR = CtrlRow[5];
    _thisInteractionData.kdR = CtrlRow[6];
    _thisInteractionData.CCPowerStateLast = CtrlRow[7];
    _thisInteractionData.WatchdogFuse = CtrlRow[8];
    
    /*Next - the current TC*/
    mysqlx::Table TCTable = DDb.getTable("CCState");
    mysqlx::RowResult TCResult = TCTable.select("UNIX_TIMESTAMP(TimeS)", "TC", "PMin", "PMax")
    .orderBy("TimeS DESC").limit(1).execute();
    /*The row with the result*/
    mysqlx::Row TCRow = TCResult.fetchOne();
    if (TCRow[0].getType() > 0) _thisInteractionData.LastCCTime = (long) TCRow[0];
    _thisInteractionData.curTemp = TCRow[1];
    _thisInteractionData.PMin = TCRow[2];
    _thisInteractionData.PMax = TCRow[3];


    /*Next - the current TC from LakeShore RTD*/
    mysqlx::Table LSHTable = DDb.getTable("LSHState");
    mysqlx::RowResult LSHResult = LSHTable.select("UNIX_TIMESTAMP(TimeS)", "LSHTemp")
            .orderBy("TimeS DESC").limit(1).execute();
    /*The row with the result*/
    mysqlx::Row LSHRow = LSHResult.fetchOne();
    if (LSHRow[0].getType() > 0) _thisInteractionData.LastLSHTime = (long) LSHRow[0];
    _thisInteractionData.curTempLSH = LSHRow[1];    
    
    
    
    /*Now update the monitoring table*/
    
    // Accessing an existing table
    mysqlx::Table SMStats = DDb.getTable("SMState");
    unsigned int warnings;
    
    // Insert SQL Table data
    mysqlx::Result SMStatsResult= SMStats.insert("PID", "SystemState", "ShouldBeState")
    .values(this->ThisRunPIDValue, (int)this->CurrentFSMState, (int)this->ShouldBeFSMState).execute();
    warnings=SMStatsResult.getWarningsCount();
    
    
    
    
    // Accessing an existing table
    mysqlx::Table SendControl = DDb.getTable("ControlParameters");
    
    // Insert SQL Table data
    mysqlx::Result SCResult= SendControl.update().set("HeaterPW",this->ThisRunHeaterPower).where("IDX=1").execute();
    warnings+=SCResult.getWarningsCount();


    //Update the CCPowerOutput
    this->SentCCPower = this->ThisRunCCPower + _thisInteractionData.PMin;
    SCResult= SendControl.update().set("CCPower",this->SentCCPower).where("IDX=1").execute();
    warnings+=SCResult.getWarningsCount();

    
    //Update the CCPowerState if needed;
    if (_thisInteractionData.CCPowerStateLast != this->CCoolerPower){
        SCResult= SendControl.update().set("CCPowerState",this->CCoolerPower).where("IDX=1").execute();
        warnings+=SCResult.getWarningsCount();
        
        //Debug
        printf("Cryocooler switch %d\n",this->CCoolerPower);
        
    }


        
    
    
    if (warnings != 0) std::cout<<"SQL Generated warnings! \n";
    
    
    DDroneSession.close();
    
}

