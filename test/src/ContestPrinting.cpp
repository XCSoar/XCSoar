/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Printing.hpp"
#include "OS/FileUtil.hpp"
#include "Contest/ContestManager.hpp"
#include "Trace/Trace.hpp"

#include <fstream>

void
PrintHelper::contestmanager_print(const ContestManager &man,
                                  const Trace &trace_full,
                                  const Trace &trace_triangle,
                                  const Trace &trace_sprint)
{
  Directory::Create(Path(_T("output/results")));

  {
    std::ofstream fs("output/results/res-olc-trace.txt");
    TracePointVector v;
    trace_full.GetPoints(v);

    for (auto it = v.begin(); it != v.end(); ++it)
      fs << it->GetLocation().longitude << " " << it->GetLocation().latitude
         << " " << it->GetAltitude() << " " << it->GetTime()
         << "\n";
  }

  {
    std::ofstream fs("output/results/res-olc-trace_triangle.txt");

    TracePointVector v;
    trace_triangle.GetPoints(v);

    for (auto it = v.begin(); it != v.end(); ++it)
      fs << it->GetLocation().longitude << " " << it->GetLocation().latitude
         << " " << it->GetAltitude() << " " << it->GetTime()
         << "\n";
  }

  {
    std::ofstream fs("output/results/res-olc-trace_sprint.txt");

    TracePointVector v;
    trace_sprint.GetPoints(v);

    for (auto it = v.begin(); it != v.end(); ++it)
      fs << it->GetLocation().longitude << " " << it->GetLocation().latitude
         << " " << it->GetAltitude() << " " << it->GetTime()
         << "\n";
  }

  std::ofstream fs("output/results/res-olc-solution.txt");

  if (man.stats.solution[0].empty()) {
    fs << "# no solution\n";
    return;
  }

  if (man.stats.result[0].time > 0) {

    for (auto it = man.stats.solution[0].begin();
         it != man.stats.solution[0].end(); ++it) {
      fs << it->GetLocation().longitude << " " << it->GetLocation().latitude
         << " " << it->GetTime()
         << "\n";
    }
  }
}

void 
PrintHelper::print(const ContestResult& score)
{
  std::cout << "#   score " << score.score << "\n";
  std::cout << "#   distance " << score.distance/1000. << " (km)\n";
  std::cout << "#   speed " << score.GetSpeed() * 3.6 << " (kph)\n";
  std::cout << "#   time " << score.time << " (sec)\n";
}
