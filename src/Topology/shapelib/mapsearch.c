#include "mapprimitive.h"

#include <math.h>

#define LASTVERT(v,n)  ((v) == 0 ? n-2 : v-1)
#define NEXTVERT(v,n)  ((v) == n-2 ? 0 : v+1)

/*
** Returns MS_TRUE if rectangles a and b overlap
*/
int msRectOverlap(const rectObj *a, const rectObj *b)
{
  if(a->minx > b->maxx) return(MS_FALSE);
  if(a->maxx < b->minx) return(MS_FALSE);
  if(a->miny > b->maxy) return(MS_FALSE);
  if(a->maxy < b->miny) return(MS_FALSE);
  return(MS_TRUE);
}

/*
** Returns MS_TRUE if rectangle a is contained in rectangle b
*/
int msRectContained(const rectObj *a, const rectObj *b)
{
  if(a->minx >= b->minx && a->maxx <= b->maxx)
    if(a->miny >= b->miny && a->maxy <= b->maxy)
      return(MS_TRUE);
  return(MS_FALSE);
}

int msPointInRect(const pointObj *p, const rectObj *rect)
{
  if(p->x < rect->minx) return(MS_FALSE);
  if(p->x > rect->maxx) return(MS_FALSE);
  if(p->y < rect->miny) return(MS_FALSE);
  if(p->y > rect->maxy) return(MS_FALSE);
  return(MS_TRUE);
}
