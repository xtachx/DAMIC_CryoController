/* ***********************************************
 * DAMICM CCD Temperature controller
 *
 *
 * *********************************************** */

/*Std headers*/
#include <vector>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <unistd.h>


/*Custom Headers*/
#include "SerialDeviceT.hpp"
#include "LakeShoreHeater.hpp"
#include "Cryocooler.hpp"
#include <mysqlx/xdevapi.h>




int main( int argc, char** argv )
{


    int numRefreshes = 0;

    Cryocooler *SunPowerCC = new Cryocooler("/dev/SunPowerCC");
    sleep(1);

    while (true){

        SunPowerCC->GetTC();
        SunPowerCC->GetP();
        SunPowerCC->GetE();
        SunPowerCC->checkIfON();


        SunPowerCC->UpdateMysql();


        /*Refresh the cryocooler power level*/
        if (numRefreshes++%5==0){
            SunPowerCC->AdjustCryoPower();
            //numRefreshes=0;
        }

        /*We can do the On/off check every minute so as to not fry the cryocooler by turning it on and off too fast*/
        if (SunPowerCC->_newCCPowerState!=SunPowerCC->isON && numRefreshes%30==0){
            SunPowerCC->PowerOnOff(SunPowerCC->_newCCPowerState);
	    if (SunPowerCC->isON) SunPowerCC->AdjustCryoPower();
        }
        //printf("NR %d\n",numRefreshes);

        fflush(stdout);
        printf ("\rSunpower CC | TC: %.02f,  PMax: %.02f,  PMin: %.02f,  PCur (Set/Ask): %.02f (%.2f / %.02f),  isON: %d SQL: %s",
                        SunPowerCC->TC,SunPowerCC->PMax,SunPowerCC->PMin,SunPowerCC->PCurrent, SunPowerCC->PSet, SunPowerCC->PAsk,SunPowerCC->isON, SunPowerCC->SQLStatusMsg.c_str());

        sleep(1);
    }
    printf ("\n");





    //CryoCooler->WriteString("TC\r");
    //CryoCooler->WriteString("tc\r");

    delete SunPowerCC;


    return 0;
}

