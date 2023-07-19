#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#define MEM_POOL_NUM_SEGMENTS	   8
#define MEM_POOL_SEG_WORDS	  2048			        /* as unsigned long, 4x in bytes */
#define MEM_POOL_TYPE         unsigned long     /* the type of buffer, aligned for 32bit ! */
#define MEM_POOL_SEG_BYTES    (MEM_POOL_SEG_WORDS * sizeof(MEM_POOL_TYPE))
#define MEM_POOL_MEMORY_LOC   DMAMEM            /* were is the memory location for it */

void MEM_PoolInit(void);

#ifdef __cplusplus
extern "C" {
#endif
void *MEM_PoolAlloc(unsigned int size);
void MEM_PoolFree(void *mem);
#ifdef __cplusplus
}
#endif

void MEM_PoolCounters(int *inUse, int *watermark, int *max);

#endif
