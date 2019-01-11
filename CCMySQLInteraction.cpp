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
    _thisInteractionData.kp = CtrlRow[1];
    _thisInteractionData.ki = CtrlRow[2];
    _thisInteractionData.kd = CtrlRow[3];
    _thisInteractionData.kpR = CtrlRow[4];
    _thisInteractionData.kiR = CtrlRow[5];
    _thisInteractionData.kdR = CtrlRow[6];
    _thisInteractionData.CCPowerStateLast = CtrlRow[7];
    _thisInteractionData.WatchdogFuse = CtrlRow[8]
    
    /*Next - the current TC*/
    mysqlx::Table TCTable = DDb.getTable("CCState");
    mysqlx::RowResult TCResult = TCTable.select("UNIX_TIMESTAMP(TimeS)", "TC")
    .orderBy("TimeS DESC").limit(1).execute();
    /*The row with the result*/
    mysqlx::Row TCRow = TCResult.fetchOne();
    
    _thisInteractionData.curTemp = TCRow[1];
    
    //std::cout<<"TC: "<<curTemp<<"\n";
    
    
    
    
    /*Now update the monitoring table*/
    
    // Accessing an existing table
    mysqlx::Table LSHStats = DDb.getTable("SMState");
    unsigned int warnings;
    
    // Insert SQL Table data
    mysqlx::Result LSHResult= LSHStats.insert("PID", "SystemState", "ShouldBeState")
    .values(this->ThisRunPIDValue, this->CurrentFSMState, this->ShouldBeFSMState).execute();
    warnings=LSHResult.getWarningsCount();
    
    
    
    
    // Accessing an existing table
    mysqlx::Table SendControl = DDb.getTable("ControlParameters");
    
    // Insert SQL Table data
    mysqlx::Result SCResult= SendControl.update().set("HeaterPW",this->ThisRunPIDValue).where("IDX=1").execute();
    warnings+=SCResult.getWarningsCount();
    
    //Update the CCPowerState if needed;
    if (_thisInteractionData.CCPowerStateLast != this->CCoolerPower){
        //SCResult= SendControl.update().set("CCPowerState",this->CCoolerPower).where("IDX=1").execute();
        //warnings+=SCResult.getWarningsCount();
        
        //Debug
        printf("Cryocooler switch %d\n",this->CCoolerPower);
        
    }
        
    
    
    if (warnings != 0) std::cout<<"SQL Generated warnings! \n";
    
    
    DDroneSession.close();
    
}

