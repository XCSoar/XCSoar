#ifndef TEST_DEBUG_HPP
#define TEST_DEBUG_HPP

#include "Printing.hpp"
#include <fstream>
#include <iostream>
#include <getopt.h>
#include <string>
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
extern double sink_factor;
extern double climb_factor;
extern double start_alt;
extern int terrain_height;
extern bool enable_bestcruisetrack;
const char* test_name(const char* in, int task_num, int wind_num);
extern std::string replay_file;

bool parse_args(int argc, char** argv);

#endif
