
#include <string.h>
#include <stdio.h>
#include "MEM_Pool.h"

unsigned long GMEMPool[MEM_POOL_NUM_SEGMENTS * MEM_POOL_SEG_SIZE];
static int MEMPool_mgt[MEM_POOL_NUM_SEGMENTS];

int MEMPool_inUse = 0;
int MEMPool_Watermark = 0;

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
      MEMPool_inUse++;
      if (MEMPool_Watermark < MEMPool_inUse)
        MEMPool_Watermark = MEMPool_inUse;
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
		if (mem == (void*)(&GMEMPool[MEM_POOL_SEG_SIZE * i])) {
			MEMPool_mgt[i] = 0;			//de-allocate, free now
      MEMPool_inUse--;
      return;
    }
	//else: not found, wrong address to release
	//set syserr
}

void MEM_PoolCounters(int *inUse, int *watermark, int *max)
{
  *inUse = MEMPool_inUse;
  *watermark = MEMPool_Watermark;
  *max = MEM_POOL_NUM_SEGMENTS;
}
