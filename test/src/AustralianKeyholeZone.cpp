/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#define USE_FREETYPE 1
#define ENABLE_OPENGL 1
#include "Engine/Task/ObservationZones/AustralianKeyholeZone.hpp"
#include "Engine/Task/ObservationZones/Boundary.hpp"
#include "Geo/GeoVector.hpp"
#include "Math/Angle.hpp"
#include "Math/FastTrig.hpp"

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

struct Args
  {
  std::string gnuplot_file;
  std::ofstream gnuplot;

  GeoPoint ref;
  fixed outer_radius;
  fixed inner_radius;
  Angle start_radial;
  Angle end_radial;

  Args(int argc, const char *argv[])
    : gnuplot_file(""),
      ref(Angle::Zero(), Angle::Zero()),
      outer_radius(10000),
      inner_radius(1000),
      start_radial(Angle::Degrees(350)),
      end_radial(Angle::Degrees(10))
    {
    for (int i = 1; i < argc; i++)
      {
      if ((strncmp(argv[i], "-p", strlen("-p")) == 0))
        {
        if (i < (argc - 1))
          {
          i++;
          this->gnuplot_file = argv[i];
          this->gnuplot.open(this->gnuplot_file);
          if (!this->gnuplot.is_open())
            std::cerr << "Could not open GNUPlot file "
                      << this->gnuplot_file
                      << std::endl;
          }
        }
      else if ((strncmp(argv[i], "-x", strlen("-x")) == 0))
        {
        if (i < (argc - 1))
          {
          i++;
          this->ref.longitude = Angle::Degrees(atof(argv[i]));
          }
        }
      else if ((strncmp(argv[i], "-y", strlen("-y")) == 0))
        {
        if (i < argc - 1)
          {
          i++;
          this->ref.latitude = Angle::Degrees(atof(argv[i]));
          }
        }
      }
    }
  };

void Test(Args &);

//------------------------------------------------------------------------------
void
Test(Args &args)
  {
  AustralianKeyholeZone *z = AustralianKeyholeZone::New(args.ref,
                                                        args.outer_radius,
                                                        args.inner_radius,
                                                        args.start_radial,
                                                        args.end_radial);

  if (args.gnuplot.is_open())
    {
    OZBoundary b = z->GetBoundary();
    args.gnuplot.precision(10);
    for (auto i = b.begin(); i != b.end(); i++)
      args.gnuplot << i->latitude.Degrees()
                   << " "
                   << i->longitude.Degrees()
                   << std::endl;
    args.gnuplot.close();
    }
  }

//------------------------------------------------------------------------------
int
main(int argc, const char *argv[])
  {
  Args args(argc, argv);

  Test(args);

  return 0;
  }
