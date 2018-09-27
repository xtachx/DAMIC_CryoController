/* ***********************************************
 * DAMICM CCD Temperature controller
 *
 *
 * *********************************************** */

/*ROOT Headers*/
#include "TH2F.h"
#include "TMath.h"
#include "TApplication.h"
#include "TCanvas.h"

#include "Math/Minimizer.h"
#include "Math/Factory.h"
#include "Math/Functor.h"
#include "TError.h"
#include "TParameter.h"
#include "TFile.h"
#include "TTree.h"
#include "TArrayD.h"
#include "TF1.h"
#include "TGraph.h"
#include <TSystem.h>


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

    Cryocooler *SunPowerCC = new Cryocooler("/dev/ttyUSB0");
    sleep(1);

    for (int i=0; i<1000; i++){

        SunPowerCC->GetTC();
        SunPowerCC->GetP();
        SunPowerCC->GetE();
        SunPowerCC->checkIfON();


        SunPowerCC->UpdateMysql();


        /*If the numrefreshes % 300 ==0 i.e. 5 minutes, then reset the cryocooler power level*/
        if (numRefreshes++%300==0){
            SunPowerCC->AdjustCryoPower();
            //numRefreshes=0;
        }
        //printf("NR %d\n",numRefreshes);

        fflush(stdout);
        printf ("\rSunpower CC | TC: %.02f,  PMax: %.02f,  PMin: %.02f,  PCur: %.02f,  isON: %d SQL: %s",
                        SunPowerCC->TC,SunPowerCC->PMax,SunPowerCC->PMin,SunPowerCC->PCurrent,SunPowerCC->isON, SunPowerCC->SQLStatusMsg.c_str());

        sleep(1);
    }
    printf ("\n");





    //CryoCooler->WriteString("TC\r");
    //CryoCooler->WriteString("tc\r");

    delete SunPowerCC;


    return 0;
}

