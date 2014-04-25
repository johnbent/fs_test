#ifndef IOD_H
#define IOD_H

#ifdef HAS_IOD

#include "iod_api.h"
#include "iod_types.h"

#include "utilities.h"

struct Parameters;
struct State;

int init_iod( struct Parameters *p, struct State *s );
int iod_open( struct Parameters *, struct State *, char *target, int read_write );

#endif


#endif
