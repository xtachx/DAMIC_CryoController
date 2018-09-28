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
#include <mysqlx/xdevapi.h>
#include <cmath>        // std::abs

#include "PID_v1.h"



/*Function to interact with MySQL*/


void UpdateMysql(double &kp, double &ki, double &kd, double &TTemp, double &curTemp, double PID){

    // Connect to server using a connection URL
    mysqlx::Session DDroneSession("localhost", 33060, "DAMICDrone", "Modane00");
    mysqlx::Schema DDb = DDroneSession.getSchema("DAMICDrone");

    /*First lets get the control parameters*/
    mysqlx::Table CtrlTable = DDb.getTable("ControlParameters");
    mysqlx::RowResult ControlResult = CtrlTable.select("TTemperature", "Kp", "Ki", "Kd")
      .bind("IDX", 1).execute();
    /*The row with the result*/
    mysqlx::Row CtrlRow = ControlResult.fetchOne();

    TTemp = CtrlRow[0];
    kp = CtrlRow[1];
    ki = CtrlRow[2];
    kd = CtrlRow[3];

    /*Next - the current TC*/
    mysqlx::Table TCTable = DDb.getTable("CCState");
    mysqlx::RowResult TCResult = TCTable.select("UNIX_TIMESTAMP(TimeS)", "TC")
      .orderBy("TimeS DESC").limit(1).execute();
    /*The row with the result*/
    mysqlx::Row TCRow = TCResult.fetchOne();

    curTemp = TCRow[1];

    //std::cout<<"TC: "<<curTemp<<"\n";




    /*Now update the monitoring table*/

    // Accessing an existing table
    mysqlx::Table LSHStats = DDb.getTable("PIDState");

    // Insert SQL Table data
    mysqlx::Result LSHResult= LSHStats.insert("PID")
           .values(PID).execute();

    unsigned int warnings;
    warnings=LSHResult.getWarningsCount();


    // Accessing an existing table
    mysqlx::Table SendControl = DDb.getTable("ControlParameters");

    // Insert SQL Table data
    mysqlx::Result SCResult= SendControl.update().set("HeaterPW",PID).where("IDX=1").execute();
    warnings+=SCResult.getWarningsCount();


    if (warnings != 0) std::cout<<"SQL Generated warnings! \n";


    DDroneSession.close();

}



int main( int argc, char** argv )
{


    /*The __CINT__ checks if ROOT environment is present. We need that
    *to plot the histograms.*/
    #ifndef __CINT__
    /*In their infinite wisdom, a ROOT TApplication needs a argc and an argv even
     *if you dont have one. So, supply them fake ones.*/


    //Define Variables we'll be connecting to
    double Setpoint, Input, Output;




    //Specify the links and initial tuning parameters
    double Kp=2, Ki=5, Kd=1;
    PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, P_ON_M, DIRECT);

    Setpoint = 150;
    Input = 273.00;
    double absOutput;

    //turn the PID on
    myPID.SetMode(AUTOMATIC);


    for (int i=0; i<1000; i++)
    {
        //std::cout<<"New temperature: ";
        //std::cin>>Input;


        myPID.Compute();

        UpdateMysql(Kp,Ki,Kd, Setpoint, Input, std::abs(Output));
        //std::cout<<"\nPW: "<<std::abs(Output)<<"\n";

        if (Kp != myPID.GetKp() || Ki != myPID.GetKi() || Kd != myPID.GetKd()){
            printf("Tuning change!\n");
            myPID.SetTunings(Kp,Ki,Kd);
        }

        fflush(stdout);
        printf ("\rPIDControl | Time: %d  TC: %.02f  PID: %0.2f",
                std::time(0), Input, std::abs(Output));

        sleep(1);

    }


#endif // __CINT__



    return 0;
}

