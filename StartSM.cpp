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
    
    for (int i=0; i<1000; i++){
        
        DAMICM_CCSM.SMEngine();


        /*Finally, print the status*/
        //fflush(stdout);
        printf ("SMControl | Time: %ld  TC: %.02f Set %.02f PID: %0.2f SystemState %d ShouldBeState: %d\n",
            std::time(0), DAMICM_CCSM.CurrentTemperature, DAMICM_CCSM.TSetpoint, DAMICM_CCSM.ThisRunPIDValue, DAMICM_CCSM.CurrentFSMState, DAMICM_CCSM.ShouldBeFSMState);
    
        sleep(1);
    }
    


    return 0;
}

