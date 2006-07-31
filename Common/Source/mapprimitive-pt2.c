//#include "map.h"
/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005  

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
#include "stdafx.h"
#include "mapprimitive.h"
#include "maperror.h"
#include <string.h>

#include <windows.h>

typedef enum {CLIP_LEFT, CLIP_MIDDLE, CLIP_RIGHT} CLIP_STATE;

#define CLIP_CHECK(min, a, max) ((a) < (min) ? CLIP_LEFT : ((a) > (max) ? CLIP_RIGHT : CLIP_MIDDLE));
#define ROUND(a)       ( (a) + 0.5 )
#define SWAP( a, b, t) ( (t) = (a), (a) = (b), (b) = (t) )
#define EDGE_CHECK( x0, x, x1) ((x) < MS_MIN( (x0), (x1)) ? CLIP_LEFT : ((x) > MS_MAX( (x0), (x1)) ? CLIP_RIGHT : CLIP_MIDDLE ))

#define INFINITY	(1.0e+30)
#define NEARZERO	(1.0e-30)	/* 1/INFINITY */

void msFree(void *p) {
  if(p) free(p);
}

/*
** Free memory allocated for a character array
*/
void msFreeCharArray(char **array, int num_items)
{
  int i;

  if((num_items < 0) || !array) return;

  for(i=0;i<num_items;i++)
    free(array[i]);
  msFree(array);

  return;
}

void msPrintShape(shapeObj *p)
{
  int i,j;

  msDebug("Shape contains %d parts.\n",  p->numlines);
  for (i=0; i<p->numlines; i++) {
    msDebug("\tPart %d contains %d points.\n", i, p->line[i].numpoints);
    for (j=0; j<p->line[i].numpoints; j++) {
      msDebug("\t\t%d: (%f, %f)\n", j, p->line[i].point[j].x, p->line[i].point[j].y);
    }
  }
}

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

int msCopyShape(shapeObj *from, shapeObj *to) {
  int i;

  if(!from || !to) return(-1);

  for(i=0; i<from->numlines; i++)
    msAddLine(to, &(from->line[i])); // copy each line

  to->type = from->type;

  to->bounds.minx = from->bounds.minx;
  to->bounds.miny = from->bounds.miny;
  to->bounds.maxx = from->bounds.maxx;
  to->bounds.maxy = from->bounds.maxy;

  if(from->text) to->text = _strdup(from->text);

  to->classindex = from->classindex;
  to->index = from->index;
  to->tileindex = from->tileindex;

  if(from->values) {
    to->values = (char **)malloc(sizeof(char *)*from->numvalues);
    for(i=0; i<from->numvalues; i++)
      to->values[i] = _strdup(from->values[i]);
    to->numvalues = from->numvalues;
  }

  return(0);
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

void msComputeBounds(shapeObj *shape)
{
  int i, j;

  if(shape->numlines <= 0) return;
  if(shape->line[0].numpoints <= 0) return;

  shape->bounds.minx = shape->bounds.maxx = shape->line[0].point[0].x;
  shape->bounds.miny = shape->bounds.maxy = shape->line[0].point[0].y;

  for( i=0; i<shape->numlines; i++ ) {
    for( j=0; j<shape->line[i].numpoints; j++ ) {
      shape->bounds.minx = MS_MIN(shape->bounds.minx, shape->line[i].point[j].x);
      shape->bounds.maxx = MS_MAX(shape->bounds.maxx, shape->line[i].point[j].x);
      shape->bounds.miny = MS_MIN(shape->bounds.miny, shape->line[i].point[j].y);
      shape->bounds.maxy = MS_MAX(shape->bounds.maxy, shape->line[i].point[j].y);
    }
  }
}

int msAddLine(shapeObj *p, lineObj *new_line)
{
  int c, v;
  lineObj *extended_line;

  /* Create an extended line array */
  if((extended_line = (lineObj *)malloc((p->numlines + 1) * sizeof(lineObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "msAddLine()");
    return(-1);
  }

  /* Copy the old line into the extended line array */
  for (c= 0; c < p->numlines; c++)
    extended_line[c]= p->line[c];

  /* Copy the new line onto the end of the extended line array */
  c= p->numlines;
  extended_line[c].numpoints = new_line->numpoints;
  if((extended_line[c].point = (pointObj *)malloc(new_line->numpoints * sizeof(pointObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "msAddLine()");
    return(-1);
  }

  for (v= 0; v < new_line->numpoints; v++)
    extended_line[c].point[v]= new_line->point[v];

  /* Dispose of the old line */
  if(p->line) free(p->line);

  /* Update the polygon information */
  p->numlines++;
  p->line = extended_line;

  return(0);
}

/*
** Converts a rect array to a shapeObj structure. Note order is CW assuming y origin
** is in the lower left corner (normal cartesian coordinate system). Also polygon is
** is closed (i.e. first=last). This conforms to the shapefile specification. For image
** coordinate systems (i.e. GD) this is back-ass-ward, which is fine cause the function
** that calculates direction assumes min y = lower left, this way it'll still work. Drawing
** functions are independent of direction. Orientation problems can cause some nasty bugs.
*/
void msRectToPolygon(rectObj rect, shapeObj *poly)
{
  lineObj line={0,NULL};

  line.point = (pointObj *)malloc(sizeof(pointObj)*5);

  line.point[0].x = rect.minx;
  line.point[0].y = rect.miny;
  line.point[1].x = rect.minx;
  line.point[1].y = rect.maxy;
  line.point[2].x = rect.maxx;
  line.point[2].y = rect.maxy;
  line.point[3].x = rect.maxx;
  line.point[3].y = rect.miny;
  line.point[4].x = line.point[0].x;
  line.point[4].y = line.point[0].y;

  line.numpoints = 5;

  msAddLine(poly, &line);
  poly->type = MS_SHAPE_POLYGON;
  poly->bounds = rect;
  free(line.point);
}

/*
** Private implementation of the Sutherland-Cohen algorithm. Taken in part
** from "Getting Graphic: Programming Fundamentals in C and C++" by Mark Finlay
** and John Petritis. (pages 179-182)
*/
 int clipLine(double *x1, double *y1, double *x2, double *y2, rectObj rect)
{
  double x, y;
  double slope;
  CLIP_STATE check1, check2;

  if(*x1 < rect.minx && *x2 < rect.minx)
    return(MS_FALSE);
  if(*x1 > rect.maxx && *x2 > rect.maxx)
    return(MS_FALSE);

  check1 = CLIP_CHECK(rect.minx, *x1, rect.maxx);
  check2 = CLIP_CHECK(rect.minx, *x2, rect.maxx);
  if(check1 == CLIP_LEFT || check2 == CLIP_LEFT) {
    slope = (*y2 - *y1)/(*x2 - *x1);
    y = *y1 + (rect.minx - *x1)*slope;
    if(check1 == CLIP_LEFT) {
      *x1 = rect.minx;
      *y1 = y;
    } else {
      *x2 = rect.minx;
      *y2 = y;
    }
  }
  if(check1 == CLIP_RIGHT || check2 == CLIP_RIGHT) {
    slope = (*y2 - *y1)/(*x2 - *x1);
    y = *y1 + (rect.maxx - *x1)*slope;
    if(check1 == CLIP_RIGHT) {
      *x1 = rect.maxx;
      *y1 = y;
    } else {
      *x2 = rect.maxx;
      *y2 = y;
    }
  }

  if(*y1 < rect.miny && *y2 < rect.miny)
    return(MS_FALSE);
  if(*y1 > rect.maxy && *y2 > rect.maxy)
    return(MS_FALSE);

  check1 = CLIP_CHECK(rect.miny, *y1, rect.maxy);
  check2 = CLIP_CHECK(rect.miny, *y2, rect.maxy);
  if(check1 == CLIP_LEFT || check2 == CLIP_LEFT) {
    slope = (*x2 - *x1)/(*y2 - *y1);
    x = *x1 + (rect.miny - *y1)*slope;
    if(check1 == CLIP_LEFT) {
      *x1 = x;
      *y1 = rect.miny;
    } else {
      *x2 = x;
      *y2 = rect.miny;
    }
  }
  if(check1 == CLIP_RIGHT || check2 == CLIP_RIGHT) {
    slope = (*x2 - *x1)/(*y2 - *y1);
    x = *x1 + (rect.maxy - *y1)*slope;
    if(check1 == CLIP_RIGHT) {
      *x1 = x;
      *y1 = rect.maxy;
    } else {
      *x2 = x;
      *y2 = rect.maxy;
    }
  }

  return(MS_TRUE);
}

/*
** Routine for clipping a polyline, stored in a shapeObj struct, to a
** rectangle. Uses clipLine() function to create a new shapeObj.
*/
void msClipPolylineRect(shapeObj *shape, rectObj rect)
{
  int i,j;
  lineObj line={0,NULL};
  double x1, x2, y1, y2;
  shapeObj tmp={0,NULL};

  if(shape->numlines == 0) /* nothing to clip */
    return;

  for(i=0; i<shape->numlines; i++) {

    line.point = (pointObj *)malloc(sizeof(pointObj)*shape->line[i].numpoints);
    line.numpoints = 0;

    x1 = shape->line[i].point[0].x;
    y1 = shape->line[i].point[0].y;
    for(j=1; j<shape->line[i].numpoints; j++) {
      x2 = shape->line[i].point[j].x;
      y2 = shape->line[i].point[j].y;

      if(clipLine(&x1,&y1,&x2,&y2,rect) == MS_TRUE) {
	if(line.numpoints == 0) { /* first segment, add both points */
	  line.point[0].x = x1;
	  line.point[0].y = y1;
	  line.point[1].x = x2;
	  line.point[1].y = y2;
	  line.numpoints = 2;
	} else { /* add just the last point */
	  line.point[line.numpoints].x = x2;
	  line.point[line.numpoints].y = y2;
	  line.numpoints++;
	}

	if((x2 != shape->line[i].point[j].x) || (y2 != shape->line[i].point[j].y)) {
	  msAddLine(&tmp, &line);
	  line.numpoints = 0; /* new line */
	}
      }

      x1 = shape->line[i].point[j].x;
      y1 = shape->line[i].point[j].y;
    }

    if(line.numpoints > 0)
      msAddLine(&tmp, &line);
    free(line.point);
    line.numpoints = 0; /* new line */
  }

  for (i=0; i<shape->numlines; i++) free(shape->line[i].point);
  free(shape->line);

  shape->line = tmp.line;
  shape->numlines = tmp.numlines;
}

/*
** Slightly modified version of the Liang-Barsky polygon clipping algorithm
*/
void msClipPolygonRect(shapeObj *shape, rectObj rect)
{
  int i, j;
  double deltax, deltay, xin,xout,  yin,yout;
  double tinx,tiny,  toutx,touty,  tin1, tin2,  tout;
  double x1,y1, x2,y2;

  shapeObj tmp;
  lineObj line={0,NULL};

  msInitShape(&tmp);

  if(shape->numlines == 0) /* nothing to clip */
    return;

  for(j=0; j<shape->numlines; j++) {

    line.point = (pointObj *)malloc(sizeof(pointObj)*2*shape->line[j].numpoints+1); /* worst case scenario, +1 allows us to duplicate the 1st and last point */
    line.numpoints = 0;

    for (i = 0; i < shape->line[j].numpoints-1; i++) {

      x1 = shape->line[j].point[i].x;
      y1 = shape->line[j].point[i].y;
      x2 = shape->line[j].point[i+1].x;
      y2 = shape->line[j].point[i+1].y;

      deltax = x2-x1;
      if (deltax == 0) { /* bump off of the vertical */
	deltax = (x1 > rect.minx) ? -NEARZERO : NEARZERO ;
      }
      deltay = y2-y1;
      if (deltay == 0) { /* bump off of the horizontal */
	deltay = (y1 > rect.miny) ? -NEARZERO : NEARZERO ;
      }

      if (deltax > 0) {		/*  points to right */
	xin = rect.minx;
	xout = rect.maxx;
      }
      else {
	xin = rect.maxx;
	xout = rect.minx;
      }
      if (deltay > 0) {		/*  points up */
	yin = rect.miny;
	yout = rect.maxy;
      }
      else {
	yin = rect.maxy;
	yout = rect.miny;
      }

      tinx = (xin - x1)/deltax;
      tiny = (yin - y1)/deltay;

      if (tinx < tiny) {	/* hits x first */
	tin1 = tinx;
	tin2 = tiny;
      } else {			/* hits y first */
	tin1 = tiny;
	tin2 = tinx;
      }

      if (1 >= tin1) {
	if (0 < tin1) {
	  line.point[line.numpoints].x = xin;
	  line.point[line.numpoints].y = yin;
	  line.numpoints++;
	}
	if (1 >= tin2) {
	  toutx = (xout - x1)/deltax;
	  touty = (yout - y1)/deltay;

	  tout = (toutx < touty) ? toutx : touty ;

	  if (0 < tin2 || 0 < tout) {
	    if (tin2 <= tout) {
	      if (0 < tin2) {
		if (tinx > tiny) {
		  line.point[line.numpoints].x = xin;
		  line.point[line.numpoints].y = y1 + tinx*deltay;
		  line.numpoints++;
		} else {
		  line.point[line.numpoints].x = x1 + tiny*deltax;
		  line.point[line.numpoints].y = yin;
		  line.numpoints++;
		}
	      }
	      if (1 > tout) {
		if (toutx < touty) {
		  line.point[line.numpoints].x = xout;
		  line.point[line.numpoints].y = y1 + toutx*deltay;
		  line.numpoints++;
		} else {
		  line.point[line.numpoints].x = x1 + touty*deltax;
		  line.point[line.numpoints].y = yout;
		  line.numpoints++;
		}
	      }	else {
		line.point[line.numpoints].x = x2;
		line.point[line.numpoints].y = y2;
		line.numpoints++;
	      }
	    } else {
	      if (tinx > tiny) {
		line.point[line.numpoints].x = xin;
		line.point[line.numpoints].y = yout;
		line.numpoints++;
	      } else {
		line.point[line.numpoints].x = xout;
		line.point[line.numpoints].y = yin;
		line.numpoints++;
	      }
	    }
	  }
	}
      }
    }

    if(line.numpoints > 0) {
      line.point[line.numpoints].x = line.point[0].x; // force closure
      line.point[line.numpoints].y = line.point[0].y;
      line.numpoints++;
      msAddLine(&tmp, &line);
    }

    free(line.point);
  } /* next line */

  for (i=0; i<shape->numlines; i++) free(shape->line[i].point);
  free(shape->line);

  shape->line = tmp.line;
  shape->numlines = tmp.numlines;

  return;
}

