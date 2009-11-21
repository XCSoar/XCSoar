#include "test_debug.hpp"
#include <stdlib.h>

#ifdef DO_PRINT
#include <stdio.h>
#endif

int n_samples = 0;
bool interactive = true;

#ifdef INSTRUMENT_TASK
extern long count_mc;
extern unsigned count_intersections;
extern unsigned n_queries;
extern unsigned count_distbearing;
extern unsigned num_dijkstra;
#endif


void distance_counts() {
  if (n_samples) {
#ifndef NEWTASK
    printf("# Instrumentation\n");
#ifdef INSTRUMENT_TASK
    printf("#     dist+bearing calcs/c %d\n",count_distbearing/n_samples); 
    printf("#     mc calcs/c %d\n",(int)(count_mc/n_samples));
    printf("#     dijkstra/c %d\n",num_dijkstra/n_samples);
    printf("#     intersection tests/q %d\n",count_intersections/n_queries);
    printf("#    (total queries %d)\n\n",n_queries);
#endif
    printf("#    (total cycles %d)\n\n",n_samples);
#endif
  }
  n_samples = 0;
#ifdef INSTRUMENT_TASK
  count_intersections = 0;
  n_queries = 0;
  count_distbearing = 0;
  count_mc = 0;
  num_dijkstra = 0;
#endif
}

void print_queries(unsigned n, std::ostream &fout) {
#ifdef INSTRUMENT_TASK
  fout << n << " " << count_intersections/n_queries << "\n";
  count_intersections = 0;
  n_queries = 0;
#endif
}

/** 
 * Wait-for-key prompt
 * 
 * @param time time of simulation
 * 
 * @return character received by keyboard
 */
char wait_prompt(const double time) {
  if (interactive) {
#ifdef DO_PRINT
    printf("# %g [enter to continue]\n",time);
#endif
    return getchar();
  } else {
    return 0;
  }
}


/*
  100, 1604 cycles
  my ipaq: 
  test 1: 27.7 seconds, 277ms/cycle
  test 2: 117 seconds, 72ms/cycle
  test 3: 45 seconds, 28ms/cycle

  test 1  61209: 81 ms/c
  test 2 116266: 72 ms/c
  test 3  46122: 29 ms/c
  test 4 111742: 70 ms/c

*/
