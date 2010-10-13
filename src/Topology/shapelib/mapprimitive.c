#include "map.h"
#include "mapprimitive.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

void msInitShape(shapeObj *shape)
{
  // spatial component
  shape->line = NULL;
  shape->numlines = 0;
  shape->type = MS_SHAPE_NULL;
  shape->bounds.minx = shape->bounds.miny = -1;
  shape->bounds.maxx = shape->bounds.maxy = -1;

  // attribute component
  shape->values = NULL;
  shape->numvalues = 0;

  // annotation component
  shape->text = NULL;

  // bookkeeping component
  shape->classindex = 0; // default class
  shape->tileindex = shape->index = -1;
}

void msFreeShape(shapeObj *shape)
{
  int c;

  for (c= 0; c < shape->numlines; c++)
    free(shape->line[c].point);
  free(shape->line);

  if(shape->values) msFreeCharArray(shape->values, shape->numvalues);
  if(shape->text) free(shape->text);

  msInitShape(shape); // now reset
}
