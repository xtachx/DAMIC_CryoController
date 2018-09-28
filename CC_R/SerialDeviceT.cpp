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



SerialDevice::SerialDevice(std::string port){
    /* Open File Descriptor */
    USB = open( port.c_str(), O_RDWR| O_NOCTTY );

    /* Error Handling */
    if ( USB < 0 )
    {
        std::cout << "Error " << errno << " opening " << port << ": " << strerror (errno) << std::endl;
    }


    memset (&tty, 0, sizeof(tty));

    /* Error Handling */
    if ( tcgetattr ( USB, &tty ) != 0 )
    {
        std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
    }



    /* Set Baud Rate */
    //cfsetospeed (&tty, (speed_t)B4800);
    //cfsetispeed (&tty, (speed_t)B4800);

    /* Setting other Port Stuff */
    //tty.c_cflag     &=  ~PARENB;            // Make 8n1
    //tty.c_cflag     &=  ~CSTOPB;
    //tty.c_cflag     &=  ~CSIZE;
    //tty.c_cflag     |=  CS8;


    tty.c_cc[VMIN]   =  0;                  // read doesn't block
    tty.c_cc[VTIME]  =  10;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&tty);
    //tty.c_cflag     &=  ~CRTSCTS;           // no flow control

    /* Flush Port, then applies attributes */
    //tcflush( USB, TCIFLUSH );
    //if ( tcsetattr ( USB, TCSANOW, &tty ) != 0)
    //{
    //    std::cout << "Error " << errno << " from tcsetattr" << std::endl;
    //}

}


SerialDevice::~SerialDevice()
{
    close(USB);
}



void SerialDevice::WriteString(std::string s)
{

    char scmd[s.size()];
    s.copy(scmd, s.size());


    int n_written;
    n_written = write( USB, scmd, sizeof(scmd)  );

    //printf ("Written bits: %d\n",n_written);
}


std::string SerialDevice::ReadLine()
{


    int n = 0,
        spot = 0;
    char buf = '\0';
    int i = 0;

    /* Whole response*/
    char response[1024];
    memset(response, '\0', sizeof(response));

    do
    {
        n = read( USB, &buf, 1 );

        sprintf( &response[spot], "%c", buf );
        spot += n;
    }
    while( n > 0 && buf != '\n');

    if (n < 0)
    {
        //std::cout << "Error reading: " << strerror(errno) << std::endl;
        return "";
    }
    else if (n == 0)
    {
        //std::cout << "Read nothing!" << std::endl;
        return "";
    }
    else
    {
        //std::cout << "Response: " << response << std::endl;
        return response;
    }


}

