/* **************************************************************************************
 * Code containing the CCD class that will have the entire definitions we will need
 * such as the erase procedure, the set biases and the CCD tuning etc
 *
 * by Pitam Mitra 2018 for DAMIC-M
 * **************************************************************************************/


#ifndef Serial_HPP_INCLUDED
#define Serial_HPP_INCLUDED


/*Includes*/
#include <iostream>


#include <boost/asio.hpp>

class SerialDevice
{
public:
    SerialDevice(std::string port, unsigned int baud_rate);
    ~SerialDevice();
    void WriteString(std::string s);
    std::string ReadLine();

private:
    boost::asio::io_service io;
    boost::asio::serial_port serial;
};

#endif
