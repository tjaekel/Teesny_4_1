
#include <string.h>
#include <stdio.h>
#include "MEM_Pool.h"

unsigned long GMEMPool[MEM_POOL_NUM_SEGMENTS * MEM_POOL_SEG_SIZE];

static int MEMPool_mgt[MEM_POOL_NUM_SEGMENTS];

void MEM_PoolInit(void) {
  memset(MEMPool_mgt, 0, sizeof(MEMPool_mgt));
}

void *MEM_PoolAlloc(int size)
{
	int i;
	for (i = 0; i < MEM_POOL_NUM_SEGMENTS; i++)
		if (MEMPool_mgt[i] == 0)
		{
			MEMPool_mgt[i] = 1;			//allocated, in use now
			return &GMEMPool[MEM_POOL_SEG_SIZE * i];
		}
	//set syserr
	return NULL;						//not available, out of memory
}

void MEM_PoolFree(void *mem)
{
	int i;
	//find the segment and release it
	for (i = 0; i < MEM_POOL_NUM_SEGMENTS; i++)
		if (mem == (void*)(&GMEMPool[MEM_POOL_SEG_SIZE * i]))
			MEMPool_mgt[i] = 0;			//de-allocate, free now
	//else: not found, wrong address to release
	//set syserr
}
