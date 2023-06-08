#ifndef __CMD_THREAD_H__
#define __CMD_THREAD_H__H

void CMD_loop(void);
void CMD_setup(void);

/* helper functions */
#ifdef __cplusplus
extern "C" {
#endif
void CMD_delay(int ms);
#ifdef __cplusplus
}
#endif

#endif
