#include "OLCSprint.hpp"

#include <fstream>


OLCSprint::OLCSprint(OnlineContest& _olc):
  OLCDijkstra(_olc, 4, 0) 
{

}

bool 
OLCSprint::admit_candidate(const ScanTaskPoint &candidate) const
{
  return (get_point(candidate).time <=
          solution[0].time + 9000) &&
    OLCDijkstra::admit_candidate(candidate);
}

fixed
OLCSprint::score()  
{
  static const fixed fixed_9000(9000);

  const fixed dist = OLCDijkstra::score();

  if (positive(dist)) {
    const fixed time(solution[num_stages-1].time-solution[0].time);

    if (positive(time)) {

      const fixed speed = dist/time;
      printf("%d %d %g\n", dist.as_int(), time.as_int(), speed.as_double());

      std::ofstream fs("results/res-olc-sprint.txt");
      for (unsigned i=0; i<num_stages; ++i) {
        fs << solution[i].get_location().Longitude << " " << solution[i].get_location().Latitude 
           << " " << solution[i].altitude << " " << solution[i].time 
           << "\n";
      }

      return speed;
    }
  }
  return fixed_zero;

//  return dist/max(fixed_9000, time);
}
