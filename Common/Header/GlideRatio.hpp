#ifndef XCSOAR_GLIDE_RATIO_HPP
#define XCSOAR_GLIDE_RATIO_HPP

#include "Defines.h"

typedef struct {
        int     distance[MAXLDROTARYSIZE]; // rotary array with a predefined max capacity
        int     altitude[MAXLDROTARYSIZE];
	int	totaldistance;
        short   start;          // pointer to current first item in rotarybuf if used
        short   size;           // real size of rotary buffer (0-size)
	bool	valid;
} ldrotary_s;

bool	InitLDRotary(ldrotary_s *buf);
void	InsertLDRotary(ldrotary_s *buf, int distance, int altitude);
int	CalculateLDRotary(ldrotary_s *buf);

#endif
