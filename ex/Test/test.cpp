#include "Math/FastMath.h"
#include "harness_flight.hpp"

int main(int argc, char** argv) {
  ::InitSineTable();

  // default arguments
  verbose=1;  
  
  if (!parse_args(argc,argv)) {
    return 0;
  }
  test_flight(5,0,1.0,false);
  test_flight(5,0,1.0,true);
}

