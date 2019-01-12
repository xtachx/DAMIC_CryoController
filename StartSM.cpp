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
#include <cmath>        // std::abs

#include "CryoControlSM.hpp"




int main( int argc, char** argv )
{
    
    CryoControlSM DAMICM_CCSM;
    
    while(true){
        
        DAMICM_CCSM.SMEngine();


        /*Finally, print the status*/
        fflush(stdout);
        printf ("\rSMControl | Time: %ld  (T/R: %.02f/%.03f) (Set T/R %.02f/%.03f) PID: %0.2f State %d ShouldBe %d ",
            std::time(0), DAMICM_CCSM.CurrentTemperature, DAMICM_CCSM.TempratureRateMovingAvg, DAMICM_CCSM.SetTemperature, DAMICM_CCSM.RSetpoint, DAMICM_CCSM.ThisRunPIDValue, DAMICM_CCSM.CurrentFSMState, DAMICM_CCSM.ShouldBeFSMState);
    
        sleep(1);
    }
    


    return 0;
}

