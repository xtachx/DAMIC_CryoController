#include "rpc/client.h"
#include <iostream>
#include <string>
using std::string;

int main() {
  rpc::client c("localhost", 20555);
      c.call("PerformSweep");

}
