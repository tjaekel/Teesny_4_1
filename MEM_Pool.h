#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#define MEM_POOL_NUM_SEGMENTS	8
#define MEM_POOL_SEG_SIZE	2048			/* as unsigned long, 4x */

void MEM_PoolInit(void);

#ifdef __cplusplus
extern "C" {
#endif
void *MEM_PoolAlloc(int size);
void MEM_PoolFree(void *mem);
#ifdef __cplusplus
}
#endif

#endif
