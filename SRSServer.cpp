#include "rpc/server.h"
#include <string>
#include <iostream>
#include "SRSPowerSupply.hpp"


int main()
{

    /*Create the RPC Server*/
    rpc::server srv(20555);

    /*Create the SRS Server Object and bind the member functions to the server*/
    SRSPowerSupply *SRS1 = new SRSPowerSupply("/dev/ttyUSB3")


    srv.bind("PerformSweep", [&SRS1](){ SRS1.PerformSweep(); });


    srv.run();
    return 0;
}
