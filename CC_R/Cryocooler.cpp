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
#include "Cryocooler.hpp"

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
    printf ("Sunpower CryoCooler is initialized and ready to accept instructions.\n");

}


Cryocooler::~Cryocooler()
{
    close(USB);
}



void Cryocooler::GetTC(void)
{

    std::string CCTC_String, CCTC_Firstline;
    std::string CCCMd = "TC\r";

    this->WriteString(CCCMd);
    //Read first line
    CCTC_Firstline = this->ReadLine();
    //TC is in second line
    CCTC_String = this->ReadLine();
    this->TC = std::stof(CCTC_String);

}


void Cryocooler::GetP(void){

    std::string CC_String, CC_Firstline;
    std::string CCCMd = "P\r";

    this->WriteString(CCCMd);
    //Read first line
    CC_Firstline = this->ReadLine();
    //TC is in second line
    CC_String = this->ReadLine();
    this->PCurrent = std::stof(CC_String);

}

void Cryocooler::GetE(void){

    std::string CC_S1, CC_S2, CC_S3, CC_Firstline;
    std::string CCCMd = "E\r";

    this->WriteString(CCCMd);
    //Read first line
    CC_Firstline = this->ReadLine();

    CC_S1 = this->ReadLine();
    this->PMax = std::stof(CC_S1);
    CC_S2 = this->ReadLine();
    this->PMin = std::stof(CC_S2);
    CC_S3 = this->ReadLine();
    this->PSet = std::stof(CC_S3);

}

void Cryocooler::checkIfON(void){

    std::string CC_String, CC_Firstline;
    std::string CCCMd = "SET SSTOP\r";

    this->WriteString(CCCMd);
    //Read first line
    CC_Firstline = this->ReadLine();
    //TC is in second line
    CC_String = this->ReadLine();
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
        //Read first line
        CC_Firstline = this->ReadLine();
        //SSTOP Status is in second line
        CC_String = this->ReadLine();
        sstop_status = std::stod(CC_String);

        if (sstop_status == 1) this->isON = false;
        else this->isON = true;

    /*This next case is not trivial!!*/
    } else if (newPowerState==0) {

        CCCMd = "SET SSTOP=1\r";
        this->WriteString(CCCMd);
        /*These are the set sstop command repeated*/
        this->ReadLine();
        this->ReadLine();

        /*Now the BS controller will generate few lines until it gets to "Complete"*/
        do {
            CC_String = this->ReadLineThrowR();
        } while (CC_String.compare(CC_String.size()-9,9,"COMPLETE.") !=0);

        /*Clear the extra line*/
        this->ReadLine();
        //CC_Firstline = this->ReadLine();

        /*Check if cryocooler is stopped*/
        CCCMd = "SET SSTOP\r";
        this->WriteString(CCCMd);

        CC_Firstline = this->ReadLine();
        CC_String = this->ReadLine();
        sstop_status = std::stod(CC_String);

        if (sstop_status == 1) this->isON = false;
        else this->isON = true;

    } else {
        std::cout<<"New power state is unknown. Not changing power state."<<std::endl;
        this->_newCCPowerState = this->isON;
    }
}

void Cryocooler::AdjustCryoPower(void){

    int minP = (int) this->PMin;

    std::string CC_String, CC_Firstline;
    std::string CCCMd = "SET PWOUT="+std::to_string(minP)+"\r";

    this->WriteString(CCCMd);
    //Read first line
    CC_Firstline = this->ReadLine();
    //TC is in second line
    CC_String = this->ReadLine();
    int newPwout = std::stod(CC_String);

    this->PAsk = this->PMin;
    


}

void Cryocooler::UpdateMysql(void){
    // Connect to server using a connection URL
    mysqlx::Session DDroneSession("localhost", 33060, "DAMICDrone", "Modane00");
    mysqlx::Schema DDb = DDroneSession.getSchema("DAMICDrone");


    /*First lets get the control parameters*/
    mysqlx::Table CtrlTable = DDb.getTable("ControlParameters");
    mysqlx::RowResult ControlResult = CtrlTable.select("CCPowerState")
      .bind("IDX", 1).execute();
    /*The row with the result*/
    mysqlx::Row CtrlRow = ControlResult.fetchOne();


    this->_newCCPowerState = CtrlRow[0];




    // Accessing an existing table
    mysqlx::Table CCStats = DDb.getTable("CCState");

    // Insert SQL Table data

    mysqlx::Result CCQResult= CCStats.insert("TC", "PMin", "PMax", "PCur", "PoweredON")
           .values(std::to_string(this->TC), std::to_string(this->PMin), std::to_string(this->PMax), std::to_string(this->PCurrent), std::to_string(this->isON)).execute();

    unsigned int warnings;
    warnings=CCQResult.getWarningsCount();

    if (warnings == 0) this->SQLStatusMsg = "OK";
    else (SQLStatusMsg = "WARN!\n");
    DDroneSession.close();

}


