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


//float GetTCryo(SerialDevice **CCooler)
//{
//
//    SerialDevice* CryoCooler = *CCooler;
//
//
//    CryoCooler->WriteString("TC\r");
//    sleep(1);
//    CryoCooler->ReadLine();
//    std::string TempReading = CryoCooler->ReadLine();
//
//    return std::stof(TempReading);
//
//}



int main( int argc, char** argv )
{


    /*The __CINT__ checks if ROOT environment is present. We need that
    *to plot the histograms.*/
    #ifndef __CINT__
    /*In their infinite wisdom, a ROOT TApplication needs a argc and an argv even
     *if you dont have one. So, supply them fake ones.*/
    //static int fakeArgc = 1;
    //static char *fakeArgv[] = { "foo.exe", "-x", "myfile", "-f", "myflag", NULL};
    //TApplication* rootPlotApp = new TApplication("AA", &fakeArgc, fakeArgv);



    /*Plot Name is the first argument*/
    //std::string cCooler = "/dev/ttyUSB0";

    //Cryocooler *SunPowerCC = new Cryocooler("/dev/ttyUSB0");
    LakeShore *LSHeater = new LakeShore("/dev/ttyUSB1");
    sleep(1);

    for (int i=0; i<2; i++){

        //LSHeater->ReadPower();

        LSHeater->UpdateMysql();



        sleep(1);
        //std::cout<<"Current power setting: "<<LSHeater->currentPW<<"\n";
        //fflush(stdout);
        //printf ("\rSunpower CC | TC: %.02f,  PMax: %.02f,  PMin: %.02f,  PCur: %.02f,  isON: %d",SunPowerCC->TC,SunPowerCC->PMax,SunPowerCC->PMin,SunPowerCC->PCurrent,SunPowerCC->isON);

    }
    printf ("\n");



    #endif // __CINT__

    //CryoCooler->WriteString("TC\r");
    //CryoCooler->WriteString("tc\r");

    delete SunPowerCC;
    //delete LSHeater;


    return 0;
}

