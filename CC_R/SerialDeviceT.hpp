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
#include <termios.h>

class SerialDevice
{
public:
    SerialDevice(std::string );
    ~SerialDevice();
    void WriteString(std::string s);
    std::string ReadLine();
    std::string ReadLineThrowR();


    int USB;
    struct termios tty;

};

#endif
