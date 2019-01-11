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
#include <cmath>        // std::abs


#include "CryoControlSM.hpp"
#include "CCMySQLInteraction.cpp"
#include "PID_v1.h"


CryoControlSM::CryoControlSM(void){

    /*Current starting state for FSM is idle. SHould be state idle*/
    CurrentFSMState = ST_Idle;
    ShouldBeFSMState = ST_Idle;

    CurrentTemperature=0;
    LastTemperature=0;
    TimeStamp=std::time(0);
    TRate=0;

    /*The jump table to different states - implementation*/
    this->STFnTable={
        {ST_Idle, &CryoControlSM::Idle},
        {ST_CoolDownHot, &CryoControlSM::CoolDownHot},
        {ST_CoolDownCold, &CryoControlSM::CoolDownCold},
        {ST_Warmup, &CryoControlSM::Warmup},
        {ST_MaintainWarm, &CryoControlSM::MaintainWarm},
        {ST_MaintainCold, &CryoControlSM::MaintainCold}
    };

    //The two PIDs
    PID AbsPID(&CurrentTemperature, &TOutput, &TSetpoint, KpA, KiA, KdA, P_ON_M, DIRECT);
    PID RatePID(&TempratureRateMovingAvg, &ROutput, &RSetpoint, KpR, KiR, KdR, P_ON_M, DIRECT);

}


void CryoControlSM::SMEngine(void ){

    /*First, run the interaction with the SQL server with updates
     *and fresh changes*/

    DataPacket _thisDataSweep;
    this->UpdateVars(_thisDataSweep); //todo:pass thisdatasweep

    /*Next look for changes*/

    /*Kp Ki Kd changes */
    if (_thisDataSweep.kpA != this->AbsPID.GetKp() || _thisDataSweep.kiA != this->AbsPID.GetKi() || _thisDataSweep.kdA != this->AbsPID.GetKd()){
        printf("Tuning change!\n");
        this->AbsPID.SetTunings(_thisDataSweep.kpA,_thisDataSweep.kiA,_thisDataSweep.kdA);
    }
    if (_thisDataSweep.kpR != this->RatePID.GetKp() || _thisDataSweep.kiR != this->RatePID.GetKi() || _thisDataSweep.kdR != this->RatePID.GetKd()){
        printf("Tuning change!\n");
        this->RatePID.SetTunings(_thisDataSweep.kpR,_thisDataSweep.kiR,_thisDataSweep.kdR);
    }

    /*Temperature set point*/
    if (_thisDataSweep.TTemp != this->TSetpoint){
        printf("Temperature setpoint change!\n");
        this->TSetpoint =_thisDataSweep.TTemp;
    }

    /*Current temperature and rate*/
    this->LastTemperature = this->CurrentTemperature;
    this->CurrentTemperature = _thisDataSweep.curTemp;
    if (this->LastTemperature !=0 ) this->TempratureRateMovingAvg = (this->CurrentTemperature-this->LastTemperature)/RateMovingAvgN - this->TempratureRateMovingAvg/RateMovingAvgN;

    /*This part decides what state the system should be in, and then switch if needed*/
    this->StateDecision();
    if (this->CurrentFSMState != this->ShouldBeFSMState) this->StateSwitch();

}


void CryoControlSM::StateDecision(void ){


    if (this->FSMMode == MANUAL) {
        this->ShouldBeFSMState=ST_Idle;
        return;
    }

    /*Warmup State conditions*/
    if (this->SetTemperature > this->CurrentTemperature + 10) this->ShouldBeFSMState=ST_Warmup;

    /*Cooldown hot*/
    if (this->SetTemperature < 220 &&
        this->SetTemperature < this->CurrentTemperature - 10 &&
        this->CurrentTemperature > 220) this->ShouldBeFSMState=ST_CoolDownHot;

    if (this->SetTemperature < 220 &&
        this->SetTemperature < this->CurrentTemperature - 10 &&
        this->CurrentTemperature <= 220) this->ShouldBeFSMState=ST_CoolDownCold;


    if (this->SetTemperature < 220 &&
        std::fabs(this->SetTemperature - this->CurrentTemperature) <= 10 &&
        this->CurrentTemperature <= 220) this->ShouldBeFSMState=ST_MaintainCold;


    if (this->SetTemperature > 220 &&
        std::fabs(this->SetTemperature - this->CurrentTemperature) <= 10 &&
        this->CurrentTemperature > 220) {
            this->ShouldBeFSMState=ST_MaintainWarm;
            if (this->SetTemperature<290)
                printf("Warning: The cryochamber has no mechanism to hold the temperature between 220 and STP.\n"
                        "The temperature will eventually stabilize at room temperature.\n");
        }


}

void CryoControlSM::StateSwitch(){


    //Switch the funtion pointer to the should be state function
    this->CryoStateFn = (this->*(this->STFnTable[this->ShouldBeFSMState].second))()

    //Switch the state of the machine
    this->CurrentFSMState = this->ShouldBeFSMState;
}




void CryoControlSM::Warmup(void){

    //Debug prints
    printf("Entered warmup state\n");

    //Entry guard function: Activate rate PID
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID.SetMode(MANUAL);
        this->RatePID.SetMode(AUTOMATIC);

        /*Set the correct rate direction for the rate*/
        this->RSetpoint = 4.5/60.0; //4.5 degrees per minute

        /*Turn cryocooler off*/
        this->CCoolerPower=0;

        /*Guard done*/
        this->EntryGuardActive = false;
    }

    //Calculate Rate PID
    this->RatePID.Compute();
    this->ThisRunPIDValue = this->ROutput;

}


void CryoControlSM::Idle(void){

    //Debug prints
    printf("Entered Idle state\n");

    //Entry guard function: Activate rate PID
    if (this->EntryGuardActive){
        this->AbsPID.SetMode(MANUAL);
        this->RatePID.SetMode(MANUAL);

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

    //Debug prints
    printf("Entered CoolDownHot state\n");

    //Entry guard function: Activate rate PID
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID.SetMode(MANUAL);
        this->RatePID.SetMode(AUTOMATIC);

        /*Set the correct rate direction for the rate*/
        this->RSetpoint = -4.5/60.0; //4.5 degrees per minute
        /*Turn cryocooler on*/
        this->CCoolerPower=1;


        this->EntryGuardActive = false;
    }

    //Calculate Rate PID
    this->RatePID.Compute();
    this->ThisRunPIDValue = this->ROutput;


}

void CryoControlSM::CoolDownCold(void){

    //Debug prints
    printf("Entered CoolDownCold state\n");

    //Entry guard function: Activate rate PID
    if (this->EntryGuardActive){

        /*Activate the correct PID*/
        this->AbsPID.SetMode(MANUAL);
        this->RatePID.SetMode(AUTOMATIC);

        /*Set the correct rate direction for the rate*/
        this->RSetpoint = -4.5/60.0; //4.5 degrees per minute
        /*Turn cryocooler on*/
        this->CCoolerPower=1;


        this->EntryGuardActive = false;
    }

    //Calculate Rate PID
    this->RatePID.Compute();
    this->ThisRunPIDValue = this->ROutput;


}


void CryoControlSM::MaintainWarm(void){

    //Debug prints
    printf("Entered MaintainWarm state\n");

    //Entry guard function: Activate rate PID
    if (this->EntryGuardActive){
        this->AbsPID.SetMode(AUTOMATIC);
        this->RatePID.SetMode(MANUAL);
        this->EntryGuardActive = false;
    }


    //Calculate Rate PID
    this->AbsPID.Compute();
    this->ThisRunPIDValue = this->TOutput;


}

void CryoControlSM::MaintainCold(void){

    //Debug prints
    printf("Entered MaintainCold state\n");

    //Entry guard function: Activate rate PID
    if (this->EntryGuardActive){
        this->AbsPID.SetMode(AUTOMATIC);
        this->RatePID.SetMode(MANUAL);
        this->EntryGuardActive = false;
    }


    //Calculate Rate PID
    this->AbsPID.Compute();
    this->ThisRunPIDValue = this->TOutput;

}


