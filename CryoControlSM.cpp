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
#include <chrono>

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
        {ST_MaintainCold, &CryoControlSM::MaintainCold},
        {ST_fault, &CryoControlSM::Fault}
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

    /*Cryocooler PMax and PMin*/
    this->PMax = _thisDataSweep.PMax;
    this->PMin = _thisDataSweep.PMin;

    /*Last sweep times*/
    this->LastCCTime = _thisDataSweep.LastCCTime;
    this->LastLSHTime = _thisDataSweep.LastLSHTime;

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

    /*Store the two temperatures independently. Measurement > 0 is there to prevent values of 0 or so if the RTD is disconnected*/
    if (_thisDataSweep.curTemp >10  this->MovingAvgCCRTD += (_thisDataSweep.curTemp - this->MovingAvgCCRTD)/RateMovingAvgN;
    if (_thisDataSweep.curTempLSH >10 ) this->MovingAvgLSHRTD += (_thisDataSweep.curTempLSH - this->MovingAvgLSHRTD)/RateMovingAvgN;



    /*Decide what state the system should be in. Then run the function to switch state if needed.*/
    this->StateDecision();
    if (this->CurrentFSMState != this->ShouldBeFSMState) this->StateSwitch();

    /*Finally, run the state function. Note: This probably should be run between decision and switch if one wants to use exit guards.*/
    (this->*CryoStateFn)();

    /*Perform post run operations and sanity checks*/
    this->PostRunSanityCheck();



}

void CryoControlSM::PostRunSanityCheck(void ){

    /*Decouple the PID computation from the cryocooler power and heater power*/
    if (this->ThisRunPIDValue<0.0){
        /*PID is negative - CC power is increased, heater is set to 0*/
        this->ThisRunHeaterPower = 0.0;
        this->ThisRunCCPower = fabs(this->ThisRunPIDValue);
    } else {
        /*PID is positive - heater gets the power, CC Power is set to 0*/
        this->ThisRunHeaterPower = this->ThisRunPIDValue;
        this->ThisRunCCPower = 0.0;
    }


    /* Catastrophe prevention
     * Condition: Temperature is over 320 C.
     * Action: Both CC Power and Heater power set to 0.
     */

    if (this->MovingAvgCCRTD > 320 || this->MovingAvgLSHRTD > 320 ){
        this->ThisRunHeaterPower = 0.0;
        this->ThisRunCCPower = 0.0;
        this->ShouldBeFSMState=ST_Fault;
    }


    /* Catastrophe condition 2
     * Either the cryocontrol or the LSH control
     * has crashed and is not responding.
     */
    time(&NowTime);
    int LastDeltaLSH = difftime(NowTime,this->LastLSHTime);
    int LastDeltaCC = difftime(NowTime,this->LastCCTime);

    if (LastDeltaLSH > 30 || LastDeltaCC > 30)
        printf("There has been no communication from LSH (%d) / Cryocooler(%d) in the last 30+ seconds!\n", LastDeltaLSH, LastDeltaCC);
    if (LastDeltaLSH > 60 || LastDeltaCC > 60){
        printf("Fault: No communication from LakeShore / CC.\n");
        this->ThisRunHeaterPower = 0.0;
        this->ThisRunCCPower = 0.0;
        this->ShouldBeFSMState=ST_Fault;
    }
    

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
        this->AbsPID->SetOutputLimits(0,75);
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
        this->AbsPID->SetOutputLimits(0,75);
        this->ThisRunCCPower = 0.0;

        /*Turn cryocooler off*/
        this->CCoolerPower=0;

        this->EntryGuardActive = false;
    }

    /*System is idle - so heater should be OFF*/
    this->ThisRunPIDValue = 0.0;

}

void CryoControlSM::Fault(void){


    /*Entry guard function: Deactivate all PIDs.
     *Turn off cryocooler.
     */
    if (this->EntryGuardActive){

        this->AbsPID->SetMode(MANUAL);
        this->RatePID->SetMode(MANUAL);

        /*Turn off the cryocooler power control feature and put cryocooler to min power*/
        this->RatePID->SetOutputLimits(0,75);
        this->AbsPID->SetOutputLimits(0,75);

        /*Turn cryocooler off*/
        this->CCoolerPower=0;

        this->EntryGuardActive = false;
    }

    /*System is at fault - so heater should be OFF*/
    this->ThisRunPIDValue = 0.0;
    this->ThisRunCCPower = 0.0;
    this->ThisRunHeaterPower = 0.0;

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

        this->RatePID->SetOutputLimits(0,75);
        this->AbsPID->SetOutputLimits(0,75);

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
        this->AbsPID->SetOutputLimits(-120,75);



        /*Set the correct rate direction for the rate*/
        this->RSetpoint = -1.0*DeltaTRatePerMin/60.0; // degrees per minute
        /*Turn cryocooler on*/
        this->CCoolerPower=1;


        this->EntryGuardActive = false;
    }

    /* Calculate Rate PID*/
    this->RatePID->Compute();
    this->ThisRunPIDValue = this->ROutput;

}


void CryoControlSM::MaintainWarm(void){


    /*Entry guard function: Activate AbsPID.
     *Turn off cryocooler.
     */
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID->SetMode(AUTOMATIC);
        this->RatePID->SetMode(MANUAL);

        this->AbsPID->SetOutputLimits(0,75);
        this->RatePID->SetOutputLimits(0,75);

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

        /*Turn off the cryocooler power control feature and put cryocooler to controlled power*/
        this->RatePID->SetOutputLimits(-120,75);
        this->AbsPID->SetOutputLimits(-120,75);

        /*Ensure cryocooler is on*/
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
