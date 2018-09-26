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
    usleep(50);
    LSP_String = this->ReadLine();

    this->currentPW = std::stof(LSP_String);

    //printf ("Written bits: %d\n",n_written);
}


