#ifndef IOD_FSTEST_TYPES_H
#define IOD_FSTEST_TYPES_H

#ifdef HAS_IOD

#include "iod_api.h"
#include "iod_types.h"

typedef struct iod_parameters_s {
	int checksum;
} iod_parameters_t;

typedef struct iod_state_s {
	iod_trans_id_t tid;
	iod_handle_t coh;
	iod_handle_t oh;
        iod_obj_id_t oid; 
	iod_obj_type_t otype;
	iod_parameters_t params;
	iod_blob_iodesc_t *io_desc;
        iod_kv_t kv;
        iod_array_struct_t *array;
        iod_hyperslab_t *slab;
	iod_mem_desc_t *mem_desc;
	iod_checksum_t *cksum;
        char *keytype;
        char hexkey[16]; // 16 chars is enough to hold 2^64 as hex
	MPI_Comm mcom;
	int myrank;
} iod_state_t;

#endif 

#endif
