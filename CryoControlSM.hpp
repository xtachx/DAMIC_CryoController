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

#define RateMovingAvgN 10


struct DataPacket{

    double kpA;
    double kiA;
    double kdA;

    double kpR;
    double kiR;
    double kdR;

    double TTemp;
    double curTemp;

    double PID;
    double SystemState;

    bool CCPowerStateLast;
    bool WatchdogFuse;

};


class CryoControlSM {

private:


    float LastTemperature;
    float TempratureRateMovingAvg;


    float SetTemperature;
    unsigned long TimeStamp;


    float TInput, TOutput, TSetpoint;
    float RInput, ROutput, RSetpoint;

    bool CCoolerPower=0;



public:

    CryoControlSM();

    /*SM Functions and states*/

    void SMEngine(void);
    void UpdateVars(DataPacket &);

    void Idle(void);
    void CoolDownHot(void );
    void CoolDownCold(void);
    void Warmup(void);

    void MaintainWarm(void);
    void MaintainCold(void);

    /*SM variables and memory*/
    double KpA=2, KiA=5, KdA=1;
    double KpR=2, KiR=5, KdR=1;

    float ThisRunPIDValue=0.0;
    float CurrentTemperature;


    /*We have two different PIDs.
     *
     *1. For controlling the absolute value of the temperature
     *   and is used around setpoints when stability is desired.
     *
     *2. For controlling the rate of ascent or descent of temperature
     *
     *I am not sure for #2 will work but we will see what happens.
     */



    /*Enum values of all the states that the FSM can be in*/
    enum FSMStates {
        ST_Idle,
        ST_CoolDownHot,
        ST_CoolDownCold,
        ST_Warmup,
        ST_MaintainWarm,
        ST_MaintainCold
    };

    /*Jump table function for the FSM states*/
    static std::map<FSMStates, void (CryoControlSM::*)( void)> STFnTable;
    void (CryoControlSM::* CryoStateFn)(void);

    FSMStates CurrentFSMState;
    FSMStates ShouldBeFSMState;

    void StateDecision( void);
    void StateSwitch (void );

    bool EntryGuardActive=false;
    bool ExitGuardActive=false;

    bool FSMMode=AUTOMATIC;

};

#endif /* CryoControlSM_hpp */
