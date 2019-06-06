//
//  CryoControlSM.cpp
//  CC_R
//
//  Created by Pitam Mitra on 1/6/19.
//  Copyright © 2019 Pitam Mitra. All rights reserved.
//
#include <vector>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <mysqlx/xdevapi.h>
#include <cmath>

#include "CryoControlSM.hpp"
#include "PID_v1.h"


CryoControlSM::CryoControlSM(void){

    /*The jump table to different states - implementation*/
    this->STFnTable={
        {ST_Idle, &CryoControlSM::Idle},
        {ST_CoolDownHot, &CryoControlSM::CoolDownHot},
        {ST_CoolDownCold, &CryoControlSM::CoolDownCold},
        {ST_Warmup, &CryoControlSM::Warmup},
        {ST_MaintainWarm, &CryoControlSM::MaintainWarm},
        {ST_MaintainCold, &CryoControlSM::MaintainCold}
    };

    /*Current starting state for FSM is idle. Should be state is also idle*/
    CurrentFSMState = ST_Idle;
    ShouldBeFSMState = ST_Idle;
    this->CryoStateFn = this->STFnTable[ShouldBeFSMState];

    CurrentTemperature=0;
    LastTemperature=0;
    TimeStamp=std::time(0);
    

    /*The two PID implementations*/
    this->AbsPID = new PID(&CurrentTemperature, &TOutput, &SetTemperature, KpA, KiA, KdA, P_ON_M, DIRECT);
    this->RatePID = new PID(&TempratureRateMovingAvg, &ROutput, &RSetpoint, KpR, KiR, KdR, P_ON_M, DIRECT);
    this->AbsPID->SetOutputLimits(0,75);
    this->RatePID->SetOutputLimits(0,75);

}

CryoControlSM::~CryoControlSM(void){
    delete this->AbsPID;
    delete this->RatePID;
}

void CryoControlSM::SMEngine(void ){

    /*First, run the interaction with the SQL server with updates
     *and fresh changes*/

    DataPacket _thisDataSweep;
    this->UpdateVars(_thisDataSweep);



    /*Next look for changes*/

    /*Kp Ki Kd changes */
    if (_thisDataSweep.kpA != this->AbsPID->GetKp() || _thisDataSweep.kiA != this->AbsPID->GetKi() || _thisDataSweep.kdA != this->AbsPID->GetKd()){
        printf("\nTuning change!\n");
        this->AbsPID->SetTunings(_thisDataSweep.kpA,_thisDataSweep.kiA,_thisDataSweep.kdA);
    }
    if (_thisDataSweep.kpR != this->RatePID->GetKp() || _thisDataSweep.kiR != this->RatePID->GetKi() || _thisDataSweep.kdR != this->RatePID->GetKd()){
        printf("\nTuning change!\n");
        this->RatePID->SetTunings(_thisDataSweep.kpR,_thisDataSweep.kiR,_thisDataSweep.kdR);
    }

    /*Temperature set point changes*/
    if (_thisDataSweep.TTemp != this->SetTemperature){
        printf("Temperature setpoint change!\n");
        this->SetTemperature = _thisDataSweep.TTemp;
    }

    /*Now update the current and the last temperature. Also update the rate of change of temperature with the new information.*/
    this->LastTemperature = this->CurrentTemperature;

    /*Logic to decide if the LSH RTD is unplugged. An unplugged RTD will show a temperature
     *of ???*/
    this->CurrentTemperature = _thisDataSweep.curTempLSH < 1 ? _thisDataSweep.curTemp : _thisDataSweep.curTempLSH;

    if (this->LastTemperature !=0 ) this->TempratureRateMovingAvg += (this->CurrentTemperature-this->LastTemperature)/RateMovingAvgN - this->TempratureRateMovingAvg/RateMovingAvgN;

    /*Decide what state the system should be in. Then run the function to switch state if needed.*/
    this->StateDecision();
    if (this->CurrentFSMState != this->ShouldBeFSMState) this->StateSwitch();

    /*Finally, run the state function. Note: This probably should be run between decision and switch if one wants to use exit guards.*/
    (this->*CryoStateFn)();

}


void CryoControlSM::StateDecision(void ){

    /*If the SM is turned off (manual mode), then the state should be idle and no output is produced.*/
    if (this->FSMMode == MANUAL) {
        this->ShouldBeFSMState=ST_Idle;
        return;
    }

    /*Warmup State conditions*/
    
    if (this->CurrentTemperature > 300 && this->SetTemperature <300)
        this->ShouldBeFSMState=ST_Idle; //this should never happen in practice. If it does, then idle.
    
    if (this->SetTemperature > this->CurrentTemperature + 10 &&
        this->CurrentTemperature < 290) this->ShouldBeFSMState=ST_Warmup;

    /*Cooldown while the current temperature is high - i.e. >220 K*/
    if (this->SetTemperature < 220 &&
        this->SetTemperature < this->CurrentTemperature - 10 &&
        this->CurrentTemperature > 220) this->ShouldBeFSMState=ST_CoolDownHot;

    /*Cooldown while the current temperature is low - i.e. <220 K*/
    if (this->SetTemperature < 220 &&
        this->SetTemperature < this->CurrentTemperature - 10 &&
        this->CurrentTemperature <= 220) this->ShouldBeFSMState=ST_CoolDownCold;

    /*Maintain a cold state once the temperature is within 10 K of set point*/
    if (this->SetTemperature < 220 &&
        std::fabs(this->SetTemperature - this->CurrentTemperature) <= 10 &&
        this->CurrentTemperature <= 220) this->ShouldBeFSMState=ST_MaintainCold;


    /*Maintain a warm state. This step explicitly turns off the cryocooler since there is no mechanism to
     *maintain a temperature between 220 K and room temperature.*/
    if (this->SetTemperature > 220 &&
        std::fabs(this->SetTemperature - this->CurrentTemperature) <= 2 &&
        this->CurrentTemperature > 220) {
            this->ShouldBeFSMState=ST_MaintainWarm;
            if (this->SetTemperature<290)
                printf("Warning: The cryochamber has no mechanism to hold the temperature between 220 and STP.\n"
                        "The temperature will eventually stabilize at room temperature.\n");
        }


}



void CryoControlSM::StateSwitch(void ){

    /*Activate entry guard*/
    this->EntryGuardActive=true;

    /*Switch the funtion pointer to the should be state function*/
    this->CryoStateFn = this->STFnTable[ShouldBeFSMState];

    /*Switch the state of the machine*/
    this->CurrentFSMState = this->ShouldBeFSMState;
}




void CryoControlSM::Warmup(void){

    /*Entry guard function: Activate rate PID. Set the rate target for RatePID.
     *Turn off cryocooler.
     */
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID->SetMode(MANUAL);
        this->RatePID->SetMode(AUTOMATIC);

        /*Turn off the cryocooler power control feature and put cryocooler to min power*/
        this->RatePID->SetOutputLimits(0,75);
        this->ThisRunCCPower = 0.0;

        /*Set the correct rate direction for the rate*/
        this->RSetpoint = DeltaTRatePerMin/60.0; //4.5 degrees per sec

        /*Turn cryocooler off*/
        this->CCoolerPower=0;

        /*Guard done*/
        this->EntryGuardActive = false;
    }

    /*Calculate Rate PID*/
    this->RatePID->Compute();
    this->ThisRunPIDValue = this->ROutput;

}


void CryoControlSM::Idle(void){


    /*Entry guard function: Deactivate all PIDs.
     *Turn off cryocooler.
     */
    if (this->EntryGuardActive){

        this->AbsPID->SetMode(MANUAL);
        this->RatePID->SetMode(MANUAL);

        /*Turn off the cryocooler power control feature and put cryocooler to min power*/
        this->RatePID->SetOutputLimits(0,75);
        this->ThisRunCCPower = 0.0;

        /*Turn cryocooler off*/
        this->CCoolerPower=0;

        this->EntryGuardActive = false;
    }

    /*System is idle - so heater should be OFF*/
    this->ThisRunPIDValue = 0.0;

}


/*Note: CooldownHot possibly requires
 *PID limits to be overriden
 *since 75% power seems to be too little
 *to get the rate at <5 / min
 */

void CryoControlSM::CoolDownHot(void ){


    /*Entry guard function: Activate rate PID. Set the rate target for RatePID.
     *Turn on cryocooler.
     */
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID->SetMode(MANUAL);
        this->RatePID->SetMode(AUTOMATIC);

        /*Set the correct rate direction for the rate*/
        this->RSetpoint = -1.0*DeltaTRatePerMin/60.0; // degrees per sec

        /*Turn cryocooler on*/
        this->CCoolerPower=1;


        this->EntryGuardActive = false;
    }

    /*Calculate Rate PID*/
    this->RatePID->Compute();
    this->ThisRunPIDValue = this->ROutput;


}

void CryoControlSM::CoolDownCold(void){



    /*Entry guard function: Activate rate PID. Set the rate target for RatePID.
     *Turn on cryocooler.
     */
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID->SetMode(MANUAL);
        this->RatePID->SetMode(AUTOMATIC);

        /*Rate PID will now go negative if it needs acceleration from the cryocooler*/
        this->RatePID->SetOutputLimits(-120,75);

        /*Set the correct rate direction for the rate*/
        this->RSetpoint = -1.0*DeltaTRatePerMin/60.0; // degrees per minute
        /*Turn cryocooler on*/
        this->CCoolerPower=1;


        this->EntryGuardActive = false;
    }

    /* Calculate Rate PID - this is the first trick to see if increasing the power
     * during cooldown works for us or not without fiddling with Kp Ki Kd.*/
    this->RatePID->Compute();

    if (this->ROutput<0.0){
        this->ThisRunPIDValue = 0.0;
        this->ThisRunCCPower = fabs(this->ROutput);
    } else {
        this->ThisRunPIDValue = this->ROutput;
        this->ThisRunCCPower = 0.0;
    }



}


void CryoControlSM::MaintainWarm(void){


    /*Entry guard function: Activate AbsPID.
     *Turn off cryocooler.
     */
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID->SetMode(AUTOMATIC);
        this->RatePID->SetMode(MANUAL);

        /*Ensure cryocooler is off*/
        this->CCoolerPower=0;

        this->EntryGuardActive = false;
    }


    /*Calculate PID*/
    this->AbsPID->Compute();
    this->ThisRunPIDValue = this->TOutput;


}

void CryoControlSM::MaintainCold(void){



    /*Entry guard function: Activate AbsPID.
     *Turn on cryocooler.
     */
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID->SetMode(AUTOMATIC);
        this->RatePID->SetMode(MANUAL);

        /*Turn off the cryocooler power control feature and put cryocooler to min power*/
        this->RatePID->SetOutputLimits(-120,75);
        

        /*Ensure cryocooler is off*/
        this->CCoolerPower=1;

        this->EntryGuardActive = false;
    }


    /*Calculate PID*/
    this->AbsPID->Compute();
    this->ThisRunPIDValue = this->TOutput;

}


/*The functions to access a copy of variables for viewing*/
double CryoControlSM::getCurrentTemperature(void) {return this->CurrentTemperature;}
double CryoControlSM::getTargetTemperature(void) {return this->SetTemperature;}
double CryoControlSM::getCurrentPIDValue(void) {return this->ThisRunPIDValue;}
double CryoControlSM::getTemperatureRate(void) {return this->TempratureRateMovingAvg;}
double CryoControlSM::getTemperatureSP(void) {return this->SetTemperature;}
double CryoControlSM::getTRateSP(void) {return this->RSetpoint;}
int CryoControlSM::getCurrentState(void) {return (int)this->CurrentFSMState;}
int CryoControlSM::getShouldBeState(void) {return (int)this->ShouldBeFSMState;}
double CryoControlSM::getSentCCPower() {return this->SentCCPower;}
