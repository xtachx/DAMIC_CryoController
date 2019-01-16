/* ***********************************************
 * DAMICM CCD Cryo Controller
 *
 * *********************************************** */

/*Std headers*/
#include <iostream>
#include <unistd.h>
#include "CryoControlSM.hpp"




int main( int argc, char** argv )
{

    CryoControlSM DAMICM_CCSM;

    while(true){

        DAMICM_CCSM.SMEngine();

        /*Finally, print the status*/
        fflush(stdout);
        printf ("\rSMControl | Time: %ld  (T/R: %.02f/%.03f) (Set T/R %.02f/%.03f) PID: %0.2f State %d ShouldBe %d ",
            std::time(0),
            DAMICM_CCSM.getCurrentTemperature(), DAMICM_CCSM.getTemperatureRate(),
            DAMICM_CCSM.getTemperatureSP(), DAMICM_CCSM.getTRateSP(),
            DAMICM_CCSM.getCurrentPIDValue(),
            DAMICM_CCSM.getCurrentState(), DAMICM_CCSM.getShouldBeState());

        sleep(1);
    }



    return 0;
}

