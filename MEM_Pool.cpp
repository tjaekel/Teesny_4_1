
#include <string.h>
#include <stdio.h>
#include "MEM_Pool.h"
#include "VCP_UART.h"
#include "SYS_error.h"

static MEM_POOL_TYPE GMEMPool[MEM_POOL_NUM_SEGMENTS * MEM_POOL_SEG_WORDS] MEM_POOL_MEMORY_LOC; /* 32bit aligned */
static int MEMPool_mgt[MEM_POOL_NUM_SEGMENTS] MEM_POOL_MEMORY_LOC;

static int MEMPool_inUse = 0;
static int MEMPool_Watermark = 0;

FLASHMEM void MEM_PoolInit(void) {
  memset(MEMPool_mgt, 0, sizeof(MEMPool_mgt));
}

/** TODO:
 * size is not used! So, we can get just one segment, not N*segments!
 */
void *MEM_PoolAlloc(unsigned int size)
{
  /* TODO: we can use last index and search from there, so that we
   * we leave some freed segments still untouched, e.g. for DMAs
   */
	int i;
  /* right now, we can only allocate one segment! */
  if (size > MEM_POOL_SEG_BYTES) {
    //set syserr
    SYSERR_Set(UART_OUT, SYSERR_MEM);
	  return NULL;						//not available, out of memory
  }

	for (i = 0; i < MEM_POOL_NUM_SEGMENTS; i++)
		if (MEMPool_mgt[i] == 0)
		{
			MEMPool_mgt[i] = 1;			//allocated, in use now
      MEMPool_inUse++;
      if (MEMPool_Watermark < MEMPool_inUse)
        MEMPool_Watermark = MEMPool_inUse;
			return &GMEMPool[MEM_POOL_SEG_WORDS * i];
		}
	//set syserr
  SYSERR_Set(UART_OUT, SYSERR_MEM);
	return NULL;						//not available, out of memory
}

void MEM_PoolFree(void *mem)
{
	int i;
	//find the segment and release it
	for (i = 0; i < MEM_POOL_NUM_SEGMENTS; i++)
		if (mem == (void*)(&GMEMPool[MEM_POOL_SEG_WORDS * i])) {
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
