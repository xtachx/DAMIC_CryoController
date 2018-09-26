/* **************************************************************************************
 * Code containing the CCD class that will have the entire definitions we will need
 * such as the erase procedure, the set biases and the CCD tuning etc
 *
 * by Pitam Mitra 2018 for DAMIC-M
 * **************************************************************************************/


#ifndef Cryocooler_HPP_INCLUDED
#define Cryocooler_HPP_INCLUDED


/*Includes*/
#include <iostream>
#include "SerialDeviceT.hpp"

class Cryocooler:public SerialDevice
{
public:
    Cryocooler(std::string);
    ~Cryocooler();

    void GetTC();
    void GetP();


    float TC;
    float PCurrent,PMax,PMin;

};

#endif
