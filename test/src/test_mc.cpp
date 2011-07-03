/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "test_debug.hpp"

#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "Navigation/Aircraft.hpp"
#include <stdio.h>
#include <fstream>
#include <string>
#include <math.h>

const fixed Vmin(5.0);

std::ofstream ofile("results/res-polar-m.txt");

static void
polar_mc(const fixed mc)
{
  GlidePolar polar(mc);
  ofile << (double)mc << " " 
        << (double)polar.GetVBestLD() << " " 
        << (double)polar.GetBestLD() << " " 
        << (double)polar.GetVMin() << " " 
        << (double)polar.GetSMin() << " " 
        << (double)polar.GetVMax() << " " 
        << (double)polar.GetSMax() << "\n";
}

static void
basic_polar(const fixed mc)
{
  char bname[100];
  sprintf(bname,"results/res-polar-%02d-best.txt",(int)(mc*10));
  std::ofstream pfile("results/res-polar.txt");
  std::ofstream mfile(bname);

  GlidePolar polar(mc);
  for (fixed V= Vmin; V<= polar.GetVMax(); V+= fixed(0.25)) {
    pfile << (double)mc << " " 
          << (double)V << " " 
          << -(double)polar.SinkRate(V) << " " 
          << (double)(V/polar.SinkRate(V))
          << "\n";
  }

  mfile << (double)mc 
        << " " << 0
        << " " << (double)mc
        << " " << (double)polar.GetBestLD() 
        << "\n";
  mfile << (double)mc 
        << " " << (double)polar.GetVBestLD() 
        << " " << -(double)polar.GetSBestLD() 
        << " " << (double)polar.GetBestLD() 
        << "\n";
}


static void
test_glide_alt(const fixed h, const fixed W, const fixed Wangle,
               std::ostream &hfile)
{
  GlidePolar polar(fixed_zero);
  polar.SetMC(fixed_one);

  AIRCRAFT_STATE ac;
  ac.wind.norm = fabs(W);
  if (negative(W)) {
    ac.wind.bearing = Angle::degrees(fixed(180)+Wangle);
  } else {
    ac.wind.bearing = Angle::degrees(Wangle);
  }
  ac.NavAltitude = h;

  GeoVector vect(fixed(400.0));
  GlideState gs(vect, fixed_zero, ac.NavAltitude, ac.wind);
  GlideResult gr = MacCready::solve(polar, gs);
  hfile << (double)h << " " 
        << (double)gr.AltitudeDifference << " "
        << (double)gr.TimeElapsed << " " 
        << (double)gr.VOpt << " " 
        << (double)W << " "
        << (double)Wangle << " "
        << "\n";
}

static void
test_glide_stf(const fixed h, const fixed W, const fixed Wangle, const fixed S,
               std::ostream &hfile)
{
  GlidePolar polar(fixed_zero);
  polar.SetMC(fixed_one);

  AIRCRAFT_STATE ac;
  ac.wind.norm = fabs(W);
  if (negative(W)) {
    ac.wind.bearing = Angle::degrees(fixed(180)+Wangle);
  } else {
    ac.wind.bearing = Angle::degrees(Wangle);
  }
  ac.NavAltitude = h;
  ac.NettoVario = S;

  GeoVector vect(fixed(400.0));
  GlideState gs(vect, fixed_zero, ac.NavAltitude, ac.wind);
  GlideResult gr = MacCready::solve(polar, gs);

  fixed Vstf = polar.SpeedToFly(ac, gr, false);

  hfile << (double)h << " " 
        << (double)gr.AltitudeDifference << " "
        << (double)gr.VOpt << " " 
        << (double)Vstf << " " 
        << (double)W << " "
        << (double)Wangle << " "
        << (double)ac.NettoVario << " "
        << "\n";
}

static bool
test_stf()
{
  { // variation with height
    std::ofstream hfile("results/res-polar-s0.txt");
    for (fixed h=fixed_zero; h<fixed(40.0); h+= fixed(0.1)) {
      test_glide_stf(h,fixed_zero,fixed_zero,fixed_zero,hfile);
    }
  }
  { // variation with S, below FG
    std::ofstream hfile("results/res-polar-s1.txt");
    for (fixed S=fixed(-4.0); S<fixed(4.0); S+= fixed(0.1)) {
      test_glide_stf(fixed_zero, fixed_zero,fixed_zero,S, hfile);
    }
  }
  { // variation with S, above FG
    std::ofstream hfile("results/res-polar-s2.txt");
    for (fixed S=fixed(-4.0); S<fixed(4.0); S+= fixed(0.1)) {
      test_glide_stf(fixed(40), fixed_zero,fixed_zero,S, hfile);
    }
  }
  { // variation with S, below FG, wind
    std::ofstream hfile("results/res-polar-s3.txt");
    for (fixed S=fixed(-4.0); S<fixed(4.0); S+= fixed(0.1)) {
      test_glide_stf(fixed_zero, fixed(10.0), fixed_zero,S, hfile);
    }
  }
  { // variation with S, above FG, wind
    std::ofstream hfile("results/res-polar-s4.txt");
    for (fixed S=fixed(-4.0); S<fixed(4.0); S+= fixed(0.1)) {
      test_glide_stf(fixed(40), fixed(10.0), fixed_zero, S, hfile);
    }
  }
  return true;
}

static bool
test_mc()
{
  for (fixed mc=fixed_zero; mc<fixed(5.0); mc+= fixed(0.1)) {
    basic_polar(mc);
    polar_mc(mc);
  }

  {
    std::ofstream hfile("results/res-polar-h-00.txt");
    for (fixed h=fixed_zero; h<fixed(40.0); h+= fixed(0.1)) {
      test_glide_alt(h, fixed_zero, fixed_zero, hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-h-50.txt");
    for (fixed h=fixed_zero; h<fixed(40.0); h+= fixed(0.1)) {
      test_glide_alt(h, fixed(5.0), fixed_zero, hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-w.txt");
    for (fixed w=fixed(-10.0); w<=fixed(10.0); w+= fixed(0.1)) {
      test_glide_alt(fixed(50.0), w, fixed_zero, hfile);
    }
  }

  {
    std::ofstream hfile("results/res-polar-a.txt");
    for (fixed a=fixed_zero; a<=fixed(360.0); a+= fixed(10)) {
      test_glide_alt(fixed(50.0), fixed(10.0), a, hfile);
    }
  }
  return true;
}

static void
test_glide_cb(const fixed h, const fixed W, const fixed Wangle,
              std::ostream &hfile)
{
  GlidePolar polar(fixed_one);

  AIRCRAFT_STATE ac;
  ac.wind.norm = fabs(W);
  if (negative(W)) {
    ac.wind.bearing = Angle::degrees(fixed(180)+Wangle);
  } else {
    ac.wind.bearing = Angle::degrees(Wangle);
  }
  ac.NavAltitude = h;

  GeoVector vect(fixed(400.0));
  GlideState gs (vect, fixed_zero, ac.NavAltitude, ac.wind);
  GlideResult gr = MacCready::solve(polar, gs);

  gr.calc_deferred(ac);

  hfile << (double)W << " "
        << (double)Wangle << " "
        << (double)gr.Vector.Bearing.value_degrees() << " "
        << (double)gr.CruiseTrackBearing.value_degrees() << " "
        << "\n";
}

static bool
test_cb()
{
  {
    std::ofstream hfile("results/res-polar-cb.txt");
    for (fixed a = fixed_zero; a <= fixed(360.0); a+= fixed(10)) {
      test_glide_cb(fixed_zero, fixed(10.0), a, hfile);
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
