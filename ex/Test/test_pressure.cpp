
#include "test_debug.hpp"
#include "Atmosphere/Pressure.hpp"

bool test_find_qnh() {
  AtmosphericPressure pres;
  pres.FindQNH(100,100);
  return fabs(pres.get_QNH()-fixed(1013.25))<0.01;
}

bool test_qnh_to_static() {
  AtmosphericPressure pres;
  fixed p0 = pres.QNHAltitudeToStaticPressure(0);
  printf("%g\n",FIXED_DOUBLE(p0));
  return fabs(p0-fixed(101325))<0.1;
}

int main(int argc, char** argv)
{

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(2);

  ok(test_find_qnh(),"find qnh",0);
  ok(test_qnh_to_static(),"qnh to static",0);

  return exit_status();
}
