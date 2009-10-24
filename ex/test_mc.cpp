////////////////////////////////////////////////

void test_mc()
{
  MacCready mc;

  AIRCRAFT_STATE ac;
  GLIDE_STATE gs;
  GLIDE_RESULT gr;

  mc.set_mc(1.0);

  ac.WindSpeed = 0.0;
  ac.WindDirection = 0;

  gs.Distance = 100;
  gs.Bearing = 0;
  gs.MinHeight = 2.0;

  ac.Altitude = 10;

  printf("AC alt %g\n", ac.Altitude);
  gr = mc.solve(ac,gs);
  gr.print(std::cout);

  ac.Altitude = 1;
  printf("AC alt %g\n", ac.Altitude);
  gr = mc.solve(ac,gs);
  gr.print(std::cout);

  ac.Altitude = 3;
  printf("AC alt %g\n", ac.Altitude);
  gr = mc.solve(ac,gs);
  gr.print(std::cout);

}
