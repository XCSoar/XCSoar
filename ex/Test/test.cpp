#include "Math/FastMath.h"
#include "harness_flight.hpp"

int main(int argc, char** argv) {
  ::InitSineTable();

  // default arguments
  
  if (!parse_args(argc,argv)) {
    return 0;
  }
  test_flight(1,0);

}

