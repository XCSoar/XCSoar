// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Printing.hpp"
#include "system/FileUtil.hpp"
#include "Contest/ContestManager.hpp"
#include "Trace/Trace.hpp"

#include <fstream>

void
PrintHelper::contestmanager_print(const ContestManager &man,
                                  const Trace &trace_full,
                                  const Trace &trace_triangle,
                                  const Trace &trace_sprint)
{
  Directory::Create(Path("output/results"));

  {
    std::ofstream fs("output/results/res-olc-trace.txt");
    TracePointVector v;
    trace_full.GetPoints(v);

    for (auto it = v.begin(); it != v.end(); ++it)
      fs << it->GetLocation().longitude << " " << it->GetLocation().latitude
         << " " << it->GetAltitude() << " " << it->GetTime().count()
         << "\n";
  }

  {
    std::ofstream fs("output/results/res-olc-trace_triangle.txt");

    TracePointVector v;
    trace_triangle.GetPoints(v);

    for (auto it = v.begin(); it != v.end(); ++it)
      fs << it->GetLocation().longitude << " " << it->GetLocation().latitude
         << " " << it->GetAltitude() << " " << it->GetTime().count()
         << "\n";
  }

  {
    std::ofstream fs("output/results/res-olc-trace_sprint.txt");

    TracePointVector v;
    trace_sprint.GetPoints(v);

    for (auto it = v.begin(); it != v.end(); ++it)
      fs << it->GetLocation().longitude << " " << it->GetLocation().latitude
         << " " << it->GetAltitude() << " " << it->GetTime().count()
         << "\n";
  }

  std::ofstream fs("output/results/res-olc-solution.txt");

  if (man.stats.solution[0].empty()) {
    fs << "# no solution\n";
    return;
  }

  if (man.stats.result[0].time.count() > 0) {

    for (auto it = man.stats.solution[0].begin();
         it != man.stats.solution[0].end(); ++it) {
      fs << it->GetLocation().longitude << " " << it->GetLocation().latitude
         << " " << it->GetTime().count()
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
  std::cout << "#   time " << score.time.count() << " (sec)\n";
}
