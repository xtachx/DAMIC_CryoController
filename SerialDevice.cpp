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

#include "SerialDevice.hpp"

#include <boost/asio.hpp>


/**
 * Constructor.
 * \param port device name, example "/dev/ttyUSB0" or "COM4"
 * \param baud_rate communication speed, example 9600 or 115200
 * \throws boost::system::system_error if cannot open the
 * serial device
 */
SerialDevice::SerialDevice(std::string port, unsigned int baud_rate)
    : io(), serial(io,port)
{
    serial.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
}


SerialDevice::~SerialDevice()
{
    serial.close();
}
/**
 * Write a string to the serial device.
 * \param s string to write
 * \throws boost::system::system_error on failure
 */
void SerialDevice::WriteString(std::string s)
{
    boost::asio::write(serial,boost::asio::buffer(s.c_str(),s.size()));
}

/**
 * Blocks until a line is received from the serial device.
 * Eventual '\n' or '\r\n' characters at the end of the string are removed.
 * \return a string containing the received line
 * \throws boost::system::system_error on failure
 */
std::string SerialDevice::ReadLine()
{
    //Reading data char by char, code is optimized for simplicity, not speed
    char c;
    std::string result;
    for(;;)
    {
        boost::asio::read(serial,boost::asio::buffer(&c,1));
        std::cout<<c<<"\n";
        switch(c)
        {
        case '\r':
            continue;
        case '\n':
            return result;
        default:
            result+=c;
        }
    }
}

