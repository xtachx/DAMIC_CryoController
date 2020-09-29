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
#include "SRSPowerSupply.hpp"
#include "MysqlCredentials.hpp"


#include <mysqlx/xdevapi.h>



SRSPowerSupply::SRSPowerSupply(std::string SerialPort) : SerialDevice(SerialPort){


    /* Set Baud Rate */
    cfsetospeed (&this->tty, (speed_t)B115200);
    cfsetispeed (&this->tty, (speed_t)B115200);

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

SRSPowerSupply::~SRSPowerSupplyController(){
	close(USB);
}


/*Remember - The imp,ementations need to be made at the very end
 *for this split template funtion!*/
template <typename T>
SRSPowerSupply::GetParameterFromSRS(std::string SRSCmd){

	this->WriteString(SRSCmd);
	std::string srsReturn = this->ReadLine();

    T ret;

	try {
        if (std::is_same<T, int>::value)
            ret = std::stoi(srsReturn);
        else if (std::is_same<T, float>)
            ret = std::stof(srsReturn);
        else if (std::is_same<T, double>)
            ret = std::stod(srsReturn);
        else if (std::is_same<T, bool>)
            ret = srsReturn=="1";
        else
            ret = srsReturn;

    } catch(...){
        printf("Error in ReadSRSOutput\n");
    }

    return ret;

}

float SRSPowerSupply::ReadPSVoltage(){
    return this->GetParameterFromSRS<float>("VOLT?\n");
}

bool SRSPowerSupply::ReadPSOutput(){
     return this->GetParameterFromSRS<int>("SOUT?\n");;
}

str::string SRSPowerSupply::IDN(){
     return this->GetParameterFromSRS<std::string>("*IDN?\n");
}

bool SRSPowerSupply::IsOVLD(){
    return this->GetParameterFromSRS<std::string>("OVLD??\n");
}

void SRSPowerSupply::WritePSVoltage(float voltage){
	std::string srsCmd = "VOLT " + std::to_string(voltage) +"\n";

	// Write to power supply
	this->WriteString(srsCmd);

	currentVoltage = this->ReadPSVoltage();
}

void SRSPowerSupply::WritePSOutput(bool output){
	std::string srsCmd = "SOUT " + std::to_string(output) + "\n";

	// Write to power supply
	this->WriteString(srsCmd);

	currentOutputStatus = this->ReadPSOutput();
}


void SRSPowerSupply::VoltageRamp(float startScanVoltage, float stopScanVoltage, float scanTime, bool display){

	// Define ramp parmaeters
	std::string srsCmd;
    srsCmd = "SCAR RANGE" + std::to_string(100) + "\n";
    this->WriteString(srsCmd);

    // Ramp start voltage
    srsCmd = "SCAB " + std::to_string(startScanVoltage) + "\n";
    this->WriteString(srsCmd);

    // Ramp stop voltage
    srsCmd = "SCAE " + std::to_string(stopScanVoltage) + "\n";
    this->WriteString(srsCmd);

    // Ramp time
    srsCmd = "SCAT " + std::to_string(scanTime) + "\n";
    this->WriteString(srsCmd);

    // Other options necessary for ramp
    this->WritePSOutput(true);
    srsCmd = "SCAD ON\n";
    this->WriteString(srsCmd);
    srsCmd = "SCAA 1\n";
    this->WriteString(srsCmd);
    srsCmd = "*TRG\n";
    this->WriteString(srsCmd);

}

void SRSPowerSupply::UpdateMysql(void){

    int _cWatchdogFuse;

    // Connect to server using a connection URL
    mysqlx::Session DDroneSession("localhost", 33060, DMysqlUser, DMysqlPass);
    mysqlx::Schema DDb = DDroneSession.getSchema("DAMICDrone");

    /*First lets get the control parameters*/
    mysqlx::Table CtrlTable = DDb.getTable("ControlParameters");
    mysqlx::RowResult ControlResult = CtrlTable.select("SRSPowerState", "WatchdogFuse")
      .bind("IDX", 1).execute();
    /*The row with the result*/
    mysqlx::Row CtrlRow = ControlResult.fetchOne();


    this->SRSPowerState = CtrlRow[0];
    this->_cWatchdogFuse = CtrlRow[1];



    /*Now update the monitoring table*/

    // Accessing an existing table
    mysqlx::Table SRSStats = DDb.getTable("SRSState");

    // Insert SQL Table data

    mysqlx::Result SRSResult= SRSStats.insert("SRSVoltage", "SRSOutStatus", "OverloadStatus", "WatchdogState")
           .values(this->currentVoltage, this->currentOutputStatus, this->OVLDStatus, WatchdogFuse).execute();

    unsigned int warnings;
    warnings=SRSResult.getWarningsCount();

    if (warnings == 0) this->SQLStatusMsg = "OK";
    else (SQLStatusMsg = "WARN!\n");


    DDroneSession.close();

}


//Perform a sweep of parameters ever couple of seconds (to be determined)
void SRSPowerSupply::PerformSweep(void){

    //Read PS voltage
    this->currentVoltage = this->ReadPSVoltage();
    //read PS output
    this->currentOutputStatus = this->ReadPSOutput();
    //read OVLD
    this->OVLDStatus = this->IsOVLD();

    std::cout<<"PS Status | Volt: "<<this->currentVoltage<<" Output: "<<this->currentOutputStatus<<" OVLD: "<<this->OVLDStatus<<"\n";


}


//
template float SRSPowerSupply::GetParameterFromSRS<float>(std::string);
template int SRSPowerSupply::GetParameterFromSRS<int>(std::string);
template bool SRSPowerSupply::GetParameterFromSRS<bool>(std::string);
template std::string SRSPowerSupply::GetParameterFromSRS<std::string>(std::string);
