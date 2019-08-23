/* **************************************************************************************
 * Code containing the SerialDevice class that will have the definitions we will need
 * for talking to the Cryocooler and Heater
 *
 * by Pitam Mitra 2018 for DAMIC-M
 * **************************************************************************************/

#include <iostream>
#include <fstream>

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
#include "Cryocooler.hpp"
#include "MysqlCredentials.hpp"


#include <mysqlx/xdevapi.h>



Cryocooler::Cryocooler(std::string port):SerialDevice(port){


    /* Set Baud Rate */
    cfsetospeed (&tty, (speed_t)B4800);
    cfsetispeed (&tty, (speed_t)B4800);

    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;
    tty.c_cflag     &=  ~CRTSCTS;           // no flow control

    /* Flush Port, then applies attributes */
    tcflush( USB, TCIFLUSH );
    if ( tcsetattr ( USB, TCSANOW, &tty ) != 0)
    {
        std::cout << "Error " << errno << " from tcsetattr" << std::endl;
    }

    /*This is needed to init the cryocooler. Why? We dont know!*/
    printf("Starting\n");
    sleep(1);
    this->WriteString("VERSION\r");
    sleep(1);
    this->ReadLine();
    this->ReadLine();
    sleep(1);

    this->PAsk=0;
    this->ControllerMode=0;
    printf ("Sunpower CryoCooler is initialized and ready to accept instructions.\n");
    this->LogFile.open("CryocoolerLog.txt", std::ios::out|std::ios::ate|std::ios::app);

    this->_hasStalled=false;

}


Cryocooler::~Cryocooler()
{
    close(USB);
    this->LogFile.close();
}


void Cryocooler::DbgWrite(std::string LogMessage){

    if (DebugMode){
        time (&this->rawtime);
        char timebuffer [80];
        struct tm * timeinfo;
        timeinfo = localtime (&this->rawtime);
        strftime (timebuffer,80,"%F-%T",timeinfo);

        this->LogFile<<timebuffer<<": "<<LogMessage<<"\n";
        }

}


int Cryocooler::GetTC(void)
{

    std::string CCTC_String, CCTC_Firstline;
    std::string CCCMd = "TC\r";

    this->WriteString(CCCMd);
    this->DbgWrite("Written TC to device.");

    //Read first line
    int NBytesRead;
    bool status;
    CCTC_Firstline = this->RReadLine(this->NReadBytes, this->_hasStalled);
    if (this->_hasStalled!=0){
            printf ("\nCryocooler stalled\n\n");
            return -1;
        }
    this->DbgWrite("TC response first line blank.");


    //TC is in second line
    CCTC_String = this->ReadLine();

    this->DbgWrite("TC response second line.");


    try {
        this->TC = std::stof(CCTC_String);
        } catch (...){
        std::cout<<"\nException in GetTC block\n\n";
        this->DbgWrite("Exception in GetTC block");
        return -2;
        }

    return 0;
}


void Cryocooler::GetP(void){

    std::string CC_String, CC_Firstline;
    std::string CCCMd = "P\r";

    this->WriteString(CCCMd);

    //this->DbgWrite("Written P to device.");


    //Read first line
    CC_Firstline = this->ReadLine();

    //this->DbgWrite("Response first line - for P.");


    //TC is in second line
    CC_String = this->ReadLine();

    //this->DbgWrite(" P response - Second line.");


    try{
        this->PCurrent = std::stof(CC_String);
    } catch (...){
        std::cout<<"\nException in GetP block\n\n";
        this->DbgWrite("Exception in GetP block");
    }

}


void Cryocooler::GetPIDState(void){

    std::string CC_String, CC_Firstline;
    std::string CCCMd = "SET PID\r";

    this->WriteString(CCCMd);
    //this->DbgWrite("Written PID to device.");



    //Read first line
    CC_Firstline = this->ReadLine();
    //this->DbgWrite("PID Response first line.");

    //TC is in second line
    CC_String = this->ReadLine();
    //this->DbgWrite(" PID response second line.");



    try{
        this->ControllerMode = std::stod(CC_String);
    } catch (...){
        std::cout<<"\nException in GetPIDState block\n\n";
        this->DbgWrite("Exception in GetPIDState block.");
    }

}



void Cryocooler::GetE(void){

    std::string CC_S1, CC_S2, CC_S3, CC_Firstline;
    std::string CCCMd = "E\r";

    this->WriteString(CCCMd);
    //this->DbgWrite("Written E to device.");



    //Read first line
    CC_Firstline = this->ReadLine();
    //this->DbgWrite(" E response first line.");


    try {
    CC_S1 = this->ReadLine();
    //this->DbgWrite("E-block: Line 2.");
    this->PMax = std::stof(CC_S1);

    CC_S2 = this->ReadLine();
    //this->DbgWrite("E-block: Line 3.");
    this->PMin = std::stof(CC_S2);


    CC_S3 = this->ReadLine();
    //this->DbgWrite("E-block: Line 4.");
    this->PSet = std::stof(CC_S3);

    } catch (...){
        std::cout<<"\nException in GetE block\n\n";
        this->DbgWrite("Exception in Get E block");
    }
}

void Cryocooler::checkIfON(void){

    std::string CC_String, CC_Firstline;
    std::string CCCMd = "SET SSTOP\r";

    this->WriteString(CCCMd);
    //this->DbgWrite("Written SSTOP to check if ON.");
    //Read first line
    CC_Firstline = this->ReadLine();
    //this->DbgWrite("checkIfOn first line.");
    //TC is in second line
    CC_String = this->ReadLine();
    //this->DbgWrite("checkIfOn second line.");
    bool sstop_status = std::stod(CC_String);

    if (sstop_status == 1) this->isON = false;
    else this->isON = true;

}

void Cryocooler::PowerOnOff(bool newPowerState){

    std::string CC_String, CC_Firstline;
    std::string CCCMd;
    bool sstop_status;

    if (newPowerState==1){

        CCCMd = "SET SSTOP=0\r";
        this->WriteString(CCCMd);
        //this->DbgWrite("Poweron command written");
        //Read first line
        CC_Firstline = this->ReadLine();
        //this->DbgWrite("Poweron first line");
        //SSTOP Status is in second line
        CC_String = this->ReadLine();
        //this->DbgWrite("set sttop poweron second line.");
        sstop_status = std::stod(CC_String);

        if (sstop_status == 1) this->isON = false;
        else this->isON = true;

    /*This next case is not trivial!!*/
    } else if (newPowerState==0) {

        CCCMd = "SET SSTOP=1\r";
        this->WriteString(CCCMd);
        //this->DbgWrite("Poweroff command written.");
        /*These are the set sstop command repeated*/
        this->ReadLine();
        //this->DbgWrite("Poweroff second line");
        this->ReadLine();
        //this->DbgWrite("Poweroff third line");

        /*Now the BS controller will generate few lines until it gets to "Complete"*/
        do {
            CC_String = this->ReadLineThrowR();
        } while (CC_String.compare(CC_String.size()-9,9,"COMPLETE.") !=0);

        this->DbgWrite("Controller is done throwing the BS garbage that it does here");

        /*Clear the extra line*/
        this->ReadLine();
        //this->DbgWrite("The random extra line after the garbage");
        //CC_Firstline = this->ReadLine();

        /*Check if cryocooler is stopped*/
        CCCMd = "SET SSTOP\r";
        this->WriteString(CCCMd);
        //this->DbgWrite("Check if cryocooler is on - SET SSTOP");

        CC_Firstline = this->ReadLine();
        //this->DbgWrite("Shutdown check - line 1");
        CC_String = this->ReadLine();
        //this->DbgWrite("Shutdown check line 2");
        try {
            sstop_status = std::stod(CC_String);
        } catch (...){
            std::cout<<"\nException in Get SSTOP block\n\n";
        }
        if (sstop_status == 1) this->isON = false;
        else this->isON = true;

    } else {
        std::cout<<"New power state is unknown. Not changing power state."<<std::endl;
        this->_newCCPowerState = this->isON;
    }
}

void Cryocooler::AdjustCryoPower(void){

    int minP = (int) this->PMin;
    int maxP = (int) this->PMax;

    int _sendCCPower;
    _sendCCPower = this->_newCCPower > minP ? this->_newCCPower : minP;
    _sendCCPower = _sendCCPower > maxP ? maxP : _sendCCPower;

    std::string CC_String, CC_Firstline;
    std::string CCCMd = "SET PWOUT="+std::to_string(_sendCCPower)+"\r";

    this->WriteString(CCCMd);
    //this->DbgWrite("Set pwout commd sent");
    //Read first line
    CC_Firstline = this->ReadLine();
    //this->DbgWrite("Set pwout first line");
    //TC is in second line
    CC_String = this->ReadLine();
    //this->DbgWrite("Set pwout second line.");
    try {
         int newPwout = std::stod(CC_String);
    } catch (...){
        std::cout<<"\nException in SetPW block\n\n";
    }
    this->PAsk = _sendCCPower;

}

void Cryocooler::SetCryoMode(void){

    std::string CC_String, CC_Firstline;
    std::string CCCMd = "SET PID=0\r";

    this->WriteString(CCCMd);
    //this->DbgWrite("SetCryoMode written");
    //Read first line
    CC_Firstline = this->ReadLine();
    //this->DbgWrite("SetCryoMode first line");
    //TC is in second line
    CC_String = this->ReadLine();
    //this->DbgWrite("SetCryoMode second line");

    try{
        this->ControllerMode = std::stod(CC_String);
    } catch (...){
        std::cout<<"\nException in SetCryoMode block\n\n";
    }

    std::cout<<"Cryocooler mode was set to 0 (power control)\n";

}

void Cryocooler::UpdateMysql(void){

    this->DbgWrite("MySQL updates begin");
    // Connect to server using a connection URL
    mysqlx::Session DDroneSession("localhost", 33060, DMysqlUser, DMysqlPass);
    mysqlx::Schema DDb = DDroneSession.getSchema("DAMICDrone");


    /*First lets get the control parameters*/
    mysqlx::Table CtrlTable = DDb.getTable("ControlParameters");
    mysqlx::RowResult ControlResult = CtrlTable.select("CCPowerState","CCPower")
      .bind("IDX", 1).execute();
    /*The row with the result*/
    mysqlx::Row CtrlRow = ControlResult.fetchOne();


    this->_newCCPowerState = CtrlRow[0];
    this->_newCCPower = CtrlRow[1];




    // Accessing an existing table
    mysqlx::Table CCStats = DDb.getTable("CCState");

    // Insert SQL Table data

    mysqlx::Result CCQResult= CCStats.insert("TC", "PMin", "PMax", "PCur", "PoweredON", "ControlMode")
           .values(std::to_string(this->TC), std::to_string(this->PMin), std::to_string(this->PMax), std::to_string(this->PCurrent), std::to_string(this->isON), std::to_string(this->ControllerMode)).execute();

    unsigned int warnings;
    warnings=CCQResult.getWarningsCount();

    if (warnings == 0) this->SQLStatusMsg = "OK";
    else (SQLStatusMsg = "WARN!\n");
    DDroneSession.close();

    this->DbgWrite("MySQL updates end");

}


