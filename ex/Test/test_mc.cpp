#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "Navigation/Aircraft.hpp"
#include <stdio.h>
#include <fstream>
#include <string>
#include <math.h>

const double Vmin=15.0;
const double Vmax=75.0;

std::ofstream ofile("results/res-polar-m.txt");

void polar_mc(const double mc) 
{
  GlidePolar polar(mc,0.0,0.0);
  ofile << mc << " " << polar.get_VbestLD() << " " << polar.get_bestLD() << "\n";
}

void basic_polar(const double mc) 
{
  char bname[100];
  sprintf(bname,"results/res-polar-%02d-best.txt",(int)(mc*10));
  std::ofstream pfile("results/res-polar.txt");
  std::ofstream mfile(bname);

  GlidePolar polar(mc,0.0,0.0);
  for (double V= Vmin; V<= Vmax; V+= 0.25) {
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


void test_glide_alt(const double h, const double W, 
                    const double Wangle, std::ostream &hfile) 
{
  GlidePolar polar(0.0,0.0,0.0);
  polar.set_mc(1.0);

  AIRCRAFT_STATE ac;
  ac.WindSpeed = fabs(W);
  if (W<0) {
    ac.WindDirection = 180+Wangle;
  } else {
    ac.WindDirection = Wangle;
  }
  ac.Altitude = h;

  GeoVector vect(400.0,0.0);
  GlideState gs (vect,0.0,ac);
  GLIDE_RESULT gr = polar.solve(gs);
  hfile << h << " " 
        << gr.AltitudeDifference << " "
        << gr.TimeElapsed << " " 
        << gr.VOpt << " " 
        << W << " "
        << Wangle << " "
        << "\n";
}


void test_mc()
{
  for (double mc=0.0; mc<5.0; mc+= 0.1) {
    basic_polar(mc);
    polar_mc(mc);
  }

  {
    std::ofstream hfile("results/res-polar-h-00.txt");
    for (double h=0.0; h<20.0; h+= 0.1) {
      test_glide_alt(h,0.0,0.0,hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-h-50.txt");
    for (double h=0.0; h<20.0; h+= 0.1) {
      test_glide_alt(h,5.0,0.0,hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-w.txt");
    for (double w=-10.0; w<=10.0; w+= 0.1) {
      test_glide_alt(50.0,w,0.0,hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-a.txt");
    for (double a=0.0; a<=360.0; a+= 10) {
      test_glide_alt(50.0,10.0,a,hfile);
    }
  }

}

int main() {
  test_mc();
}
