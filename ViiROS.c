#include "TM4C123GH6PM.h" /* necessary for core_cm4.h  */
#include <core_cm4.h> /* for __disable_irq() / __CLZ => portability */
#include "ViiROS.h"
#include "GPIO.h"

/*============================================================================*/
/*                       Thread Component Declaration                         */
/*============================================================================*/

/*
* ViiROS is designed to handle up to 32 threads + 1 idle thread
*
*       Reason: 
*       ViiROS uses a 32 Bit Mask for each ready and blocked threads
*       Priority represents the index of the uint32_t bitmask
*       Priority is the index for the bit in the mask
*         Bit:         31 30 29 ... 5 4 3 2 1 0 <-- (priority - 1)
*         Value:        0  0  0 ... 0 1 0 1 0 1 
*
*       The idle thread with priority 0 is excluded from the both masks and 
*       is always ready to run by checking ViiROS_readyMask == 0 
*       
*/
ViiROS_Thread *Active_Thread[33];

/* current thread (modified by PendSV -> volatile) */
ViiROS_Thread * volatile ViiROS_current = NULL;

/* next thread (modified by PendSV -> volatile) */
ViiROS_Thread * volatile ViiROS_next = NULL;

/* Ready thread bitmask – used by LOG2() to find highest ready thread */
static volatile uint32_t ViiROS_readyMask = 0U; 

/* Blocked thread bitmask  */
static volatile uint32_t ViiROS_blockedMask = 0U;

/*============================================================================*/
/*                        Idle Thread Declaration                             */
/*============================================================================*/

ViiROS_Thread Idle;
static uint32_t stack_Idle[80];

__root static void onIdle(void)
{
  while(1){
   __WFI(); /* Wait for interrupt and sleep until */
 
//   GPIO_WritePin(GPIO_PORTF, 2U, 1U);  // Blaue LED an
//   for(volatile int i = 0; i < 1000000; i++);  // Kurze Pause
//   GPIO_WritePin(GPIO_PORTF, 2U, 0U);  // Blaue LED aus
//   for(volatile int i = 0; i < 1000000; i++);  // Kurze Paus
  }
}

/*============================================================================*/
/*                       Function Declarations                                */
/*============================================================================*/

/*--------------------------- lastInit() -------------------------------------*/
/**
*@brief Last part for further initialization / code before ViiROS takes control
*
*@note  Defined as __weak so it won't trigger linker warning
*       Define code for lastInit in main.c 
*/
__weak void ViiROS_lastInit(void){};

/*----------------------------- Init() ---------------------------------------*/
void ViiROS_Init()
{
  /* Set PendSV priority to the lowest */
  NVIC_SetPriority(PendSV_IRQn, 0xFFU);
  
  
  ViiROS_ThreadStart(&Idle, 
                     onIdle, 
                     0U, 
                     stack_Idle, sizeof(stack_Idle));
 
  ViiROS_current = NULL;
}

/*----------------------------- Scheduler ------------------------------------*/

void ViiROS_Scheduler(void)
{
  ViiROS_Thread *current = ViiROS_current;
  ViiROS_Thread *next;
  
  if(ViiROS_readyMask == 0U) /* no thread ready = idle */
  {
    next = Active_Thread[0];
  }
  else 
  {
    uint32_t highPrio = LOG2(ViiROS_readyMask); /* ready highest prio */
    next = Active_Thread[highPrio];
  }
  
  if(current != next)
  {
    ViiROS_next = next;
    /* trigger PendSV for context switch */
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
  } 
}

/*------------------------------ Run() ---------------------------------------*/

void ViiROS_Run(void)
{
  /* last configurations before ViiROS takes over */
  //ViiROS_lastInit(); /* Add here individual code e. g. Switch interrupt,...  */
  
  __disable_irq();
  /* Give up control to ViiROS */
  //ViiROS_current = Active_Thread[0];
  ViiROS_Scheduler();
  
  __enable_irq();
  
  while(1){
    __WFI();
  }
}
/*----------------------------- BlockTime() ----------------------------------*/

/**
*@warning Call in idle thread is forbidden!
*/
void ViiROS_BlockTime(uint32_t time)
{  
  uint32_t current_bit;
  __disable_irq();
  
  if(ViiROS_current != Active_Thread[0])
  {
    current_bit = 1U << (ViiROS_current->priority - 1U);
    
    ViiROS_current->blocktime = time;
    ViiROS_readyMask &= ~current_bit;
    ViiROS_blockedMask |= current_bit;
    
    ViiROS_Scheduler();
  }
  __enable_irq();
}

/*----------------------------- blockWatch() ---------------------------------*/

/**
*@brief blockWatch manages the block time of every blocked thread
*
*@note  Iterates only over n blocked - very efficient and fast
*/
void ViiROS_blockWatch(void)
{
  
  if(ViiROS_blockedMask != 0U)
  {
    uint32_t watchSet = ViiROS_blockedMask;
    
    while (watchSet != 0U)
    {
      uint32_t prio = LOG2(watchSet);
      ViiROS_Thread *thread = Active_Thread[prio];
      --thread->blocktime;
      
      /* index = prio - 1 - array[index] */
      uint32_t watchBit = 1U << (prio - 1U); 
      watchSet &= ~watchBit;
      
      if(thread->blocktime == 0U)
      {
        ViiROS_blockedMask &= ~watchBit;
        ViiROS_readyMask |= watchBit;
      }
    }
    
  }
}

/*----------------------------- ThreadStart() --------------------------------*/

/**
*@brief Creates threads including fabricated initial hard- and software stack
*
*@note
*
*                LOW ADDRESS
*                --------------
*        --------R4 <-- sp (Process Stack Pointer == sp in thread mode)
*                R5
*                R6
*   Software stk R7
*                R8
*                R9
*                R10
*       ---------R11
*                R0
*                R1
*                R2
*   Hardware stk R3
*                R12
*                LR
*                PC
*       --------xPSR
*                --------------
*                HIGH ADDRESS
*
*@note  LR = 0xFFFFFFFD  
*       Special value for CPU to know to return from interrupt to thread.
*
*/

void ViiROS_ThreadStart(
    ViiROS_Thread *me, 
    ViiROS_ThreadHandler thread_Handler, 
    uint8_t priority, 
    void *stk_Storage, uint32_t stk_Size)
{
  /* Check if priority is in range and not already taken */
  if (priority <= 32 &&
      Active_Thread[priority] == (ViiROS_Thread*)0){
      
      /*  
        Setup the sp to the top of the stack of the thread.
        AAPCS (Procedure Call Standard): 8-Byte-alignment on ARM Cortex-M4    
      */
        
      uint32_t *sp = (uint32_t *)((((uint32_t)stk_Storage + stk_Size)/8)*8);
      /* 
        Other method for 8-byte-alignment:
        sp = (uint32_t *)((uint32_t)sp & ~7);  // 8-Byte-Alignment */
      
      /* hardware stack frame */
      *--sp = (1U << 24);        /* xPSR (Thumb-Bit = 1 if 0 = HardFault)*/
      *--sp = (uint32_t)thread_Handler; /* PC – entry point */
      *--sp = 0xFFFFFFFD;        /* LR = 0xFFFFFFFD => EXC_RETURN */
      *--sp = 0xCAFECAFE;        /* R12 */
      *--sp = 0xCAFECAFE;        /* R3 */
      *--sp = 0xCAFEBABE;        /* R2 */
      *--sp = 0xCAFEBABE;        /* R1 */
      *--sp = 0xCAFEBABE;        /* R0 */  
      /* software stack frame */
      *--sp = 0xCAFEBABE;        /* R11 */
      *--sp = 0xCAFEBABE;        /* R10 */
      *--sp = 0xCAFEBABE;        /* R9 */
      *--sp = 0xCAFEBABE;        /* R8 */
      *--sp = 0xCAFEBABE;        /* R7 */
      *--sp = 0xCAFEBABE;        /* R6 */
      *--sp = 0xCAFEBABE;        /* R5 */
      *--sp = 0xCAFEBABE;        /* R4 */
      
      me->sp = sp;

      /* Register the thread in the active thread array */
      Active_Thread[priority] = me;
      me->priority = priority;
      
      /* mark thread ready  */
      if (priority > 0)
      {
        uint32_t bit =  (1U << (priority - 1U));
        ViiROS_readyMask |= bit;
      }
  }    
} 

/*----------------------------- PendSV_Handler() -----------------------------*/

__stackless void PendSV_Handler(void)
{
  __asm volatile(
  
        /*__disable_irq();*/
       " CPSID     i                    \n"
        /*if (ViiROS_current != 0) */
       " LDR.N     R2, =ViiROS_current   \n"
       " LDR       R0, [R2]              \n"
       " CMP       R0, #0                \n"
       " BEQ       PendSV_restore        \n"   
         
        /* save R4 - R11 on stack */  
       " PUSH      {r4-r11}              \n"
        /* ViiROS_current->sp = sp; */
       " LDR       R0, [R2]              \n"     
       " MOV       R1, SP                \n"
       " STR       R1, [R0]              \n"
       /* sp = ViiROS_next->sp; */
       "PendSV_restore:                 \n"
       " LDR.N     R1, =ViiROS_next      \n"
       " LDR       R0, [R1]              \n"
       " LDR       SP, [R0]              \n"
        /* ViiROS_current = ViiROS_next; */
       " LDR       R0, [R1]              \n"
       " STR       R0, [R2]              \n"
        /* Restore R4 - R11 */
       " POP       {r4-r11}              \n"
        /* __enable_irq(); */
       " CPSIE     i                     \n"
       " BX        LR                    \n"
  );
}

//    uint32_t *sp;
//    __disable_irq();
//
//    if (ViiROS_current != 0)
//    {
//        //"PUSH {r4-r11}")
//        ViiROS_current->sp = sp;
//    }
//
//    sp = ViiROS_next->sp;
//
//    ViiROS_current = ViiROS_next;
//
//    //"POP {r4-r11}"
//  
//    __enable_irq();
//}