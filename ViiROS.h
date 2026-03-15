#ifndef VIIROS_H_
#define VIIROS_H_

#include <stdint.h>


typedef void (*ViiROS_ThreadHandler)(void);


/* @brief Thread Control Block (TCB) */ 
typedef struct {
  void *sp; /* thread stack pointer // top of the stack */
  uint8_t priority; /* thread priority */
  uint32_t blocktime; /* time thread spends in blocked state */
  /* space for more thread attributes like state, threadHandler and more */
}ViiROS_Thread;

/* Idle function  */
static void ViiROS_onIdle(void);

/* Callback: Kernel ready? >> further initialization before ViiROS_run */
void ViiROS_lastInit(void);

/* Give ViiROS control over system */
void ViiROS_Run(void);

/* Initialization */ 
void ViiROS_Init(void);

/* Scheduler */ 
void ViiROS_Scheduler(void);

/* BlockTime */
void ViiROS_BlockTime(uint32_t time);

/* Countwatch  */
void ViiROS_BlockWatch(void);


/* ViiROS Thread start */
void ViiROS_ThreadStart(
                        ViiROS_Thread *me,
                        ViiROS_ThreadHandler thread_Handler,
                        uint8_t priority, 
                        void *stk_Storage, uint32_t stk_Size);


#endif // VIIROS_H_