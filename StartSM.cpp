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
    
    DAMICM_CCSM.SMEngine();


    /*Finally, print the status*/
    //fflush(stdout);
    printf ("SMControl | Time: %ld  TC: %.02f  PID: %0.2f SystemState %d ShouldBeState: %d\n",
            std::time(0), DAMICM_CCSM.CurrentTemperature, DAMICM_CCSM.ThisRunPIDValue, DAMICM_CCSM.CurrentFSMState, DAMICM_CCSM.ShouldBeFSMState);



    return 0;
}

