#include "rpc/server.h"
#include <string>
#include <iostream>
#include <functional>
#include "SRSPowerSupply.hpp"
#include <chrono>
#include <thread>

void timer_start(std::function<void(void)> func, unsigned int interval)
{
  std::thread([func, interval]()
  { 
    while (true)
    { 
      auto x = std::chrono::steady_clock::now() + std::chrono::seconds(interval);
      func();
      std::this_thread::sleep_until(x);
    }
  }).detach();
}



int main()
{

    /*Create the RPC Server*/
    rpc::server srv(20555);

    /*Create the SRS Server Object and bind the member functions to the server*/
    SRSPowerSupply SRS1("/dev/ttyUSB3");
    
    /*This is for the 15 second updates and check-in MySQL*/
    std::function<void(void)> AutoSweepF = std::bind(&SRSPowerSupply::PerformSweep, &SRS1);
    timer_start(AutoSweepF, 15);
    


    /*Bind the server codes to RPC*/
    srv.bind("VoltageRamp", [&SRS1](float startScanV, float stopScanV, float scanT, bool display){ SRS1.VoltageRamp(startScanV,stopScanV,scanT,display); } );
    srv.bind("PerformSweep", [&SRS1](){ SRS1.PerformSweep(); } );


    srv.run();
    return 0;
}
