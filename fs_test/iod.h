#ifndef IOD_H
#define IOD_H

#ifdef HAS_IOD

#include "iod_api.h"
#include "iod_types.h"

#include "utilities.h"

#define NOASYNC NULL

struct Parameters;
struct State;

int iod_close(iod_state_t *);
int iod_fini(iod_state_t *);
int iod_getattr(iod_state_t *, struct stat *);
int iod_init(struct Parameters *p, struct State *s, MPI_Comm  );
int iod_open(struct Parameters *, struct State *, char *, int rd_wr,MPI_Comm);
int iod_persist(iod_state_t *s);
int iod_read(iod_state_t *, char *buf, size_t len,off_t, ssize_t *bytes);
int iod_sync(iod_state_t *);
int iod_trunc(iod_state_t * );
int iod_unlink(iod_state_t *);
int iod_write(iod_state_t *,char *buf, size_t len,off_t, ssize_t *bytes);
int iod_set_otype(iod_state_t *,const char *);
#endif


#endif
