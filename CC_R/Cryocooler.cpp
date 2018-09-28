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
    sleep(1);
    printf ("Flushing the serial line\n");
    this->WriteString("VERSION\r");
    sleep(1);
    this->ReadLine();
    this->ReadLine();
    sleep(1);
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


void Cryocooler::UpdateMysql(void){
    // Connect to server using a connection URL
    mysqlx::Session DDroneSession("localhost", 33060, "DAMICDrone", "Modane00");

    mysqlx::Schema DDb = DDroneSession.getSchema("DAMICDrone");

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

void Cryocooler::AdjustCryoPower(void){
}


