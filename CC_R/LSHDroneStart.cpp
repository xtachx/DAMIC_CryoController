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
#include <mysqlx/xdevapi.h>




int main( int argc, char** argv )
{



    LakeShore *LSHeater = new LakeShore("/dev/ttyUSB1");
    sleep(1);

    for (int i=0; i<1000; i++){

        LSHeater->ReadPower();
        LSHeater->ReadMode();
        LSHeater->UpdateMysql();

        if (LSHeater->_cWatchdogFuse != 1){
            fflush(stdout);
            printf("\r-------Watchdog fuse blown, system protection active.-----\n");
            LSHeater->TurnONOFF(0);
            LSHeater->WatchdogFuse = 0;
            sleep(1);
            continue;
        }


        if (LSHeater->currentPW != LSHeater->setPW){
            //printf("Heater power changed to %.2f from %.2f\n",LSHeater->_cHeaterPW,LSHeater->setPW );
            LSHeater->SetPowerLevel(LSHeater->setPW);

        }


        if (LSHeater->setMode != LSHeater->currentMode){
            //printf("Heater mode changed.\n");
            LSHeater->TurnONOFF(LSHeater->setMode);

        }



        fflush(stdout);
        printf ("\rLakeShore | PW: %.02f,  Mode (Set): %1d(%1d),  Watchdog Fuse: %.02d, MySQL: %s",
                LSHeater->currentPW, LSHeater->currentMode, LSHeater->setMode, LSHeater->WatchdogFuse, LSHeater->SQLStatusMsg.c_str());

        sleep(1);


    }
    printf ("\n");




    delete LSHeater;


    return 0;
}

