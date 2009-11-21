#ifndef TEST_DEBUG_HPP
#define TEST_DEBUG_HPP

#include <fstream>
#include <iostream>
extern "C" {
#include "tap.h"
}

extern int n_samples;
void distance_counts();
void print_queries(unsigned n, std::ostream &fout);
char wait_prompt(const double time);
extern bool interactive;

#endif
