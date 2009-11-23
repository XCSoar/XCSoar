#ifndef TEST_DEBUG_HPP
#define TEST_DEBUG_HPP

#include <fstream>
#include <iostream>
#include <getopt.h>
extern "C" {
#include "tap.h"
}

extern int n_samples;
void distance_counts();
void print_queries(unsigned n, std::ostream &fout);
char wait_prompt(const double time);
extern int interactive;
extern int verbose;
extern int output_skip;
extern double bearing_noise;
extern double target_noise;
extern double turn_speed;
bool parse_args(int argc, char** argv);

#endif
