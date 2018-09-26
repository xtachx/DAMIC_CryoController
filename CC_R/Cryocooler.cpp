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
    this->WriteString("TC\r");
    this->WriteString("TC\r");
    sleep(1);
    this->ReadLine();
    this->ReadLine();
    sleep(1);
}


Cryocooler::~Cryocooler()
{
    close(USB);
}



void Cryocooler::GetTC()
{

    std::string CCTC_String, CCTC_Firstline;
    std::string CCCMd = "TC\r";

    this->WriteString(CCCMd);
    //Read first line
    CCTC_Firstline = this->ReadLine();
    //TC is in second line
    CCTC_String = this->ReadLine();
    TC = std::stof(CCTC_String);

}



