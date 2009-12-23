#include "test_debug.hpp"

#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "Navigation/Aircraft.hpp"
#include <stdio.h>
#include <fstream>
#include <string>
#include <math.h>

const fixed Vmin= 5.0;

std::ofstream ofile("results/res-polar-m.txt");

void polar_mc(const fixed mc) 
{
  GlidePolar polar(mc);
  ofile << mc << " " << polar.get_VbestLD() << " " << polar.get_bestLD() 
        << " " << polar.get_Vmin() << " " << polar.get_Smin() 
        << " " << polar.get_Vmax() << " " << polar.get_Smax() << "\n";
}

void basic_polar(const fixed mc) 
{
  char bname[100];
  sprintf(bname,"results/res-polar-%02d-best.txt",(int)(mc*10));
  std::ofstream pfile("results/res-polar.txt");
  std::ofstream mfile(bname);

  GlidePolar polar(mc);
  for (fixed V= Vmin; V<= polar.get_Vmax(); V+= 0.25) {
    pfile << mc << " " << V << " " << -polar.SinkRate(V) << " " << V/polar.SinkRate(V)
          << "\n";
  }

  mfile << mc 
        << " " << 0
        << " " << mc
        << " " << polar.get_bestLD() 
        << "\n";
  mfile << mc 
        << " " << polar.get_VbestLD() 
        << " " << -polar.get_SbestLD() 
        << " " << polar.get_bestLD() 
        << "\n";
}


void test_glide_alt(const fixed h, const fixed W, 
                    const fixed Wangle, std::ostream &hfile) 
{
  GlidePolar polar(0.0);
  polar.set_mc(1.0);

  AIRCRAFT_STATE ac;
  ac.WindSpeed = fabs(W);
  if (W<0) {
    ac.WindDirection = 180+Wangle;
  } else {
    ac.WindDirection = Wangle;
  }
  ac.NavAltitude = h;

  GeoVector vect(400.0,0.0);
  GlideState gs (vect,0.0,ac);
  GlideResult gr = polar.solve(gs);
  hfile << h << " " 
        << gr.AltitudeDifference << " "
        << gr.TimeElapsed << " " 
        << gr.VOpt << " " 
        << W << " "
        << Wangle << " "
        << "\n";
}


void test_glide_stf(const fixed h, const fixed W, 
                    const fixed Wangle, 
                    const fixed S,
                    std::ostream &hfile) 
{
  GlidePolar polar(0.0);
  polar.set_mc(1.0);

  AIRCRAFT_STATE ac;
  ac.WindSpeed = fabs(W);
  if (W<0) {
    ac.WindDirection = 180+Wangle;
  } else {
    ac.WindDirection = Wangle;
  }
  ac.NavAltitude = h;
  ac.NettoVario = S;

  GeoVector vect(400.0,0.0);
  GlideState gs (vect,0.0,ac);
  GlideResult gr = polar.solve(gs);

  fixed Vstf = polar.speed_to_fly(ac, gr, false);

  hfile << h << " " 
        << gr.AltitudeDifference << " "
        << gr.VOpt << " " 
        << Vstf << " " 
        << W << " "
        << Wangle << " "
        << ac.NettoVario << " "
        << "\n";
}

bool test_stf()
{
  { // variation with height
    std::ofstream hfile("results/res-polar-s0.txt");
    for (fixed h=0.0; h<40.0; h+= 0.1) {
      test_glide_stf(h,0.0,0.0,0,hfile);
    }
  }
  { // variation with S, below FG
    std::ofstream hfile("results/res-polar-s1.txt");
    for (fixed S=-4.0; S<4.0; S+= 0.1) {
      test_glide_stf(0, 0.0,0.0,S, hfile);
    }
  }
  { // variation with S, above FG
    std::ofstream hfile("results/res-polar-s2.txt");
    for (fixed S=-4.0; S<4.0; S+= 0.1) {
      test_glide_stf(40,0.0,0.0,S, hfile);
    }
  }
  { // variation with S, below FG, wind
    std::ofstream hfile("results/res-polar-s3.txt");
    for (fixed S=-4.0; S<4.0; S+= 0.1) {
      test_glide_stf(0,10.0,0.0,S, hfile);
    }
  }
  { // variation with S, above FG, wind
    std::ofstream hfile("results/res-polar-s4.txt");
    for (fixed S=-4.0; S<4.0; S+= 0.1) {
      test_glide_stf(40,10.0,0.0,S, hfile);
    }
  }
  return true;
}


bool test_mc()
{
  for (fixed mc=0.0; mc<5.0; mc+= 0.1) {
    basic_polar(mc);
    polar_mc(mc);
  }

  {
    std::ofstream hfile("results/res-polar-h-00.txt");
    for (fixed h=0.0; h<40.0; h+= 0.1) {
      test_glide_alt(h,0.0,0.0,hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-h-50.txt");
    for (fixed h=0.0; h<40.0; h+= 0.1) {
      test_glide_alt(h,5.0,0.0,hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-w.txt");
    for (fixed w=-10.0; w<=10.0; w+= 0.1) {
      test_glide_alt(50.0,w,0.0,hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-a.txt");
    for (fixed a=0.0; a<=360.0; a+= 10) {
      test_glide_alt(50.0,10.0,a,hfile);
    }
  }
  return true;
}


void test_glide_cb(const fixed h, const fixed W, 
                   const fixed Wangle,
                   std::ostream &hfile) 
{
  GlidePolar polar(1.0);

  AIRCRAFT_STATE ac;
  ac.WindSpeed = fabs(W);
  if (W<0) {
    ac.WindDirection = 180+Wangle;
  } else {
    ac.WindDirection = Wangle;
  }
  ac.NavAltitude = h;

  GeoVector vect(400.0,0.0);
  GlideState gs (vect,0.0,ac);
  GlideResult gr = polar.solve(gs);

  gr.calc_deferred(ac);
  fixed cb = gr.CruiseTrackBearing;

  hfile << W << " "
        << Wangle << " "
        << gr.Vector.Bearing << " "
        << cb << " "
        << "\n";
}


bool test_cb()
{
  {
    std::ofstream hfile("results/res-polar-cb.txt");
    for (fixed a=0.0; a<=360.0; a+= 10) {
      test_glide_cb(0.0,10.0,a,hfile);
    }
  }
  return true;
}

int main() {

  plan_tests(3);

  ok(test_mc(),"mc output",0);
  ok(test_stf(),"mc stf",0);
  ok(test_cb(),"cruise bearing",0);

  return exit_status();

}
