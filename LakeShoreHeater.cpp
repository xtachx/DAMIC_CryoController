/* **************************************************************************************
 * Code containing the SerialDevice class that will have the definitions we will need
 * for talking to the Cryocooler and Heater
 *
 * by Pitam Mitra 2018 for DAMIC-M
 * **************************************************************************************/

#include <iostream>

/*For Serial IO*/
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <unistd.h>

#include "SerialDeviceT.hpp"
#include "LakeShoreHeater.hpp"
#include "MysqlCredentials.hpp"


#include <mysqlx/xdevapi.h>



LakeShore::LakeShore(std::string SerialPort) : SerialDevice(SerialPort){


    /* Set Baud Rate */
    cfsetospeed (&this->tty, (speed_t)B9600);
    cfsetispeed (&this->tty, (speed_t)B9600);

    /* Setting other Port Stuff */
    tty.c_cflag     |=  PARENB;
    tty.c_cflag     |= PARODD;
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS7;
    tty.c_cflag |= IXON ;
    tty.c_cflag     &=  ~CRTSCTS;           // no flow control

    /* Flush Port, then applies attributes */
    tcflush( USB, TCIFLUSH );
    if ( tcsetattr ( USB, TCSANOW, &tty ) != 0)
    {
        std::cout << "Error " << errno << " from tcsetattr" << std::endl;
    }


    this->WatchdogFuse = 1;
    this->setPW = 0;
    this->currentTempK = -1;

    printf("LakeShore 325 is now ready to accept instructions.\n");

}


LakeShore::~LakeShore()
{
    close(USB);
}



void LakeShore::ReadPower()
{
    std::string LSP_String;
    std::string LSCmd = "HTR? 1\r\n";


    this->WriteString(LSCmd);
    LSP_String = this->ReadLine();

    //std::cout<<this->currentPW<<"\n";
    
    try{
        this->currentPW = std::stof(LSP_String);
    } catch (...) {
        printf("Error in ReadPower. Continuing...\n ");
    }
    
}

void LakeShore::ReadTemperatureK()
{
    std::string LSP_String;
    std::string LSCmd = "KRDG?\r\n";


    this->WriteString(LSCmd);
    LSP_String = this->ReadLine();

    //std::cout<<this->currentPW<<"\n";
    
    try{
        this->currentTempK = std::stof(LSP_String);
    } catch (...) {
        printf("Error in ReadPower. Continuing...\n ");
    }
    
}

void LakeShore::ReadMode()
{
    std::string LSP_String;
    std::string LSCmd = "RANGE? 1\r\n";


    this->WriteString(LSCmd);
    LSP_String = this->ReadLine();

    try {
        this->currentMode = std::stof(LSP_String);
    } catch (...) {
        printf("Error reading current PW. Continuing... \n");
    }
}


void LakeShore::TurnONOFF(int pwstate){

    std::string LSCmd;

    if (pwstate <3 && pwstate >=0) {
        LSCmd = "RANGE 1,"+std::to_string(pwstate)+"\r\n";
        /*Now we have to set the power again*/
        this->SetPowerLevel(this->setPW);
    } else {
        LSCmd = "RANGE 1,0\r\n";
    }

    this->WriteString(LSCmd);

}

void LakeShore::SetPowerLevel(float newPowerLevel){

    /*Make sure power level is not out of bounds*/
    if (newPowerLevel < 0) newPowerLevel = 0;
    if (newPowerLevel > 100 ) newPowerLevel = 100;

    std::string LSCmd;
    LSCmd = "MOUT 1,"+std::to_string(newPowerLevel)+"\r\n";

    this->WriteString(LSCmd);

    /*Finally update the memory of which power level was set*/
    this->setPW = newPowerLevel;
}



void LakeShore::UpdateMysql(void){

    int _cWatchdogFuse;

    // Connect to server using a connection URL
    mysqlx::Session DDroneSession("localhost", 33060, DMysqlUser, DMysqlPass);
    mysqlx::Schema DDb = DDroneSession.getSchema("DAMICDrone");

    /*First lets get the control parameters*/
    mysqlx::Table CtrlTable = DDb.getTable("ControlParameters");
    mysqlx::RowResult ControlResult = CtrlTable.select("HeaterPW", "HeaterMode", "WatchdogFuse")
      .bind("IDX", 1).execute();
    /*The row with the result*/
    mysqlx::Row CtrlRow = ControlResult.fetchOne();


    this->setPW = CtrlRow[0];
    this->setMode = CtrlRow[1];
    this->_cWatchdogFuse = CtrlRow[2];



    /*Now update the monitoring table*/

    // Accessing an existing table
    mysqlx::Table LSHStats = DDb.getTable("LSHState");

    // Insert SQL Table data

    mysqlx::Result LSHResult= LSHStats.insert("HeaterPW", "LSHTemp", "SetPW", "HeaterMode", "WatchdogState")
           .values(this->currentPW, this->currentTempK, this->setPW, this->currentMode, WatchdogFuse).execute();

    unsigned int warnings;
    warnings=LSHResult.getWarningsCount();

    if (warnings == 0) this->SQLStatusMsg = "OK";
    else (SQLStatusMsg = "WARN!\n");


    DDroneSession.close();

}


