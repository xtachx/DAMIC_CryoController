//
//  CryoControlSM.hpp
//  CC_R
//
//  Created by Pitam Mitra on 1/6/19.
//  Copyright © 2019 Pitam Mitra. All rights reserved.
//

#ifndef CryoControlSM_hpp
#define CryoControlSM_hpp

#include <stdio.h>
#include "PID_v1.h"
#include <map>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iomanip>
#include <ctime>


#define RateMovingAvgN 20
#define DeltaTRatePerMin 3.0
#define CryoCoolerMaxPowerDelta 20


struct DataPacket{

    double kpA;
    double kiA;
    double kdA;

    double kpR;
    double kiR;
    double kdR;

    double TTemp;
    double curTemp;
    double curTempLSH;
    double PMin;
    double PMax;

    double PID;
    double SystemState;

    bool CCPowerStateLast;
    bool WatchdogFuse;


    time_t LastCCTime;
    time_t LastLSHTime;

};


class CryoControlSM {

private:


    double LastTemperature;
    unsigned long TimeStamp;


    double TInput, TOutput;
    double RInput, ROutput;

    bool CCoolerPower=0;

    /*SM variables and memory*/
    double KpA=2, KiA=5, KdA=1;
    double KpR=2, KiR=5, KdR=1;

    /*Process related variables*/
    double ThisRunPIDValue=0.0;
    double ThisRunCCPower=0.0;
    double ThisRunHeaterPower=0.0;
    double SentCCPower=0.0;
    double CurrentTemperature=0.0;
    double SetTemperature=0.0;
    double TempratureRateMovingAvg=0.0;
    double RSetpoint=0.0;

    double MovingAvgCCRTD = 0.0;
    double MovingAvgLSHRTD=0.0;

    double PMax = 0.0;
    double PMin = 0.0;
    time_t LastCCTime;
    time_t LastLSHTime;
    time_t NowTime;



    /* ***********************************************************
     *We have two different PIDs.
     *
     *1. For controlling the absolute value of the temperature
     *   and is used around setpoints when stability is desired.
     *
     *2. For controlling the rate of ascent or descent of temperature
     * ***********************************************************/

    PID* AbsPID;
    PID* RatePID;

    /*SM Functions and states*/
    void UpdateVars(DataPacket &);

    void Idle(void);
    void CoolDownHot(void );
    void CoolDownCold(void);
    void Warmup(void);

    void MaintainWarm(void);
    void MaintainCold(void);
    void Fault(void);

    /*Enum values of all the states that the FSM can be in*/
    enum FSMStates {
        ST_Idle,
        ST_CoolDownHot,
        ST_CoolDownCold,
        ST_Warmup,
        ST_MaintainWarm,
        ST_MaintainCold,
        ST_Fault
    };



    /*Jump table function for the FSM states and the function poiter to the current state*/
    std::map<FSMStates, void (CryoControlSM::*)( void)> STFnTable;
    void (CryoControlSM::* CryoStateFn)(void);

    FSMStates CurrentFSMState;
    FSMStates ShouldBeFSMState;

    void StateDecision( void);
    void StateSwitch (void );

    bool EntryGuardActive=false;
    bool ExitGuardActive=false;
    bool FSMMode=AUTOMATIC;


public:

    CryoControlSM();
    ~CryoControlSM(void );

    /*The SM Engine to be run at every time interval*/
    void SMEngine(void);

    /*Functions to access a copy of variables for viewing*/
    double getCurrentTemperature(void);
    double getTargetTemperature(void);
    double getCurrentPIDValue(void);
    double getTemperatureRate(void);
    double getTemperatureSP(void);
    double getTRateSP(void);
    int getCurrentState(void);
    int getShouldBeState(void);
    double getSentCCPower(void);

    void PostRunSanityCheck(void);


};

#endif /* CryoControlSM_hpp */
