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


float GetTCryo(SerialDevice **CCooler)
{

    SerialDevice* CryoCooler = *CCooler;


    CryoCooler->WriteString("TC\r");
    sleep(1);
    CryoCooler->ReadLine();
    std::string TempReading = CryoCooler->ReadLine();

    return std::stof(TempReading);

}


int main( int argc, char** argv )
{


    /*The __CINT__ checks if ROOT environment is present. We need that
    *to plot the histograms.*/
#ifndef __CINT__
    /*In their infinite wisdom, a ROOT TApplication needs a argc and an argv even
     *if you dont have one. So, supply them fake ones.*/
    static int fakeArgc = 1;
    static char *fakeArgv[] = { "foo.exe", "-x", "myfile", "-f", "myflag", NULL};
    TApplication* rootPlotApp = new TApplication("AA", &fakeArgc, fakeArgv);



    /*Plot Name is the first argument*/
    std::string cCooler = "/dev/ttyUSB0";

    SerialDevice *CryoCooler = new SerialDevice(cCooler, 4800);
    sleep(1);

    /*This is needed to init the cryocooler. Why? We dont know!*/
    CryoCooler->WriteString("TC\r");
    CryoCooler->WriteString("TC\r");
    sleep(1);
    CryoCooler->ReadLine();
    CryoCooler->ReadLine();
    sleep(1);



    // one bin = 1 second. change it to set the time scale
    float bintime = 1;

    TCanvas *c1 = new TCanvas("c1", "c1", 1000, 500);

    TH1F *ht = new TH1F("ht","Cryocooler Temperature",10,0,10*bintime);

    float TC=0;

    ht->SetMaximum(270);
    ht->SetMinimum(300);
    ht->SetStats(0);
    //ht->SetLineColor(2);
    ht->SetMarkerColor(2);
    ht->SetMarkerStyle(2);


    ht->GetXaxis()->SetTimeDisplay(1);
    ht->GetYaxis()->SetNdivisions(520);
    ht->Draw("P");




    for (int i=0; i<5000; i++)
    {

        TC = GetTCryo(&CryoCooler);
        ht->SetBinContent(i,TC);
        c1->Modified();
        c1->Update();
        gSystem->ProcessEvents();



        sleep(1);

    }



    rootPlotApp->Run();






#endif // __CINT__

    //CryoCooler->WriteString("TC\r");
    //CryoCooler->WriteString("tc\r");

    //CryoCooler->ReadLine();
    delete CryoCooler;


    return 0;
}

