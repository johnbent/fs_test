#ifndef IOD_FSTEST_TYPES_H
#define IOD_FSTEST_TYPES_H

#ifdef HAS_IOD

#include "iod_api.h"
#include "iod_types.h"

typedef struct iod_items_s {
	iod_trans_id_t iod_tid;
	iod_handle_t iod_coh;
} iod_items_t;

#endif 

#endif
