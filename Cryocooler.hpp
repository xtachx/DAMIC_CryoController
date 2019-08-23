/* **************************************************************************************
 * Code containing the CCD class that will have the entire definitions we will need
 * such as the erase procedure, the set biases and the CCD tuning etc
 *
 * by Pitam Mitra 2018 for DAMIC-M
 * **************************************************************************************/


#ifndef Cryocooler_HPP_INCLUDED
#define Cryocooler_HPP_INCLUDED

/*Defines*/
#define DebugMode 1

/*Includes*/
#include <iostream>
#include "SerialDeviceT.hpp"
#include <fstream>
#include <time.h>


class Cryocooler:public SerialDevice
{
public:
    Cryocooler(std::string);
    ~Cryocooler();

    int GetTC(void);
    void GetP(void);
    void GetE(void);
    void GetPIDState(void);
    void checkIfON(void);

    void PowerOnOff(bool );

    void UpdateMysql(void);
    void AdjustCryoPower(void);
    void SetCryoMode(void);



    float TC;
    float PCurrent,PMax,PMin, PSet;
    float PAsk;
    bool isON;
    bool _newCCPowerState;
    int _newCCPower;
    int ControllerMode; 

    bool _hasStalled;
    int NReadBytes;

    std::string SQLStatusMsg;
    std::ofstream LogFile;
    time_t rawtime;
    void DbgWrite(std::string );


};

#endif
