/**
*@file ViiROS.c
*
*@brief Kernel for ViiROS (Scheduler, Blocking, Context Switch)
*
*@note  Contains all thread related functions / data to run ViiROS
*       (Scheduler, Blocking, Context Switch)
*
*@note  Up to 32 threads [Prio: 32 - 1] + 1 Idle [Prio: 0]
*               -> Active_Thread[Prio] - 1 thread for each priority
*       Scheduling:
*       LOG2(x) (32U - __CLZ(x)) for atmoic read of 32 bit masks:
*               -> readyMask
*                  - cleared/set: Thread_start(), BlockTime(), BlockWatch()
*
*                  Bit:         31 30 29 ... 5 4 3 2 1 0 <-- (priority - 1)
*                  Value:        0  0  0 ... 0 1 0 1 0 1 
*
*               -> blockedMask
*                  - cleared/set: BlockTime(), BlockWatch()
*       ViiROS_Idle - not included in readyMask - runs if(readyMask == 0)
*
*       Functionality:
*               1. main(): gives up control by ViiROS_Run()
*               2. SysTick (1ms): 
*                   - BlockWatch(): - keeps track of blocked threads
*                                   - decrements thread->blocktime
*                                   - sets/clears readyMask / blockedMask
*                   - Scheduler():  - selects next highest ready thread to run
*                                   - triggers PendSV
*               3. PendSv: 
*                   - performs context switch 
*               4.Thread execute work
*
*@warning Idle is not allowed to call BlockTime()!!!
*/
#include "TM4C123GH6PM.h" /* necessary for core_cm4.h  */
#include <core_cm4.h> /* for __disable_irq() / __CLZ => portability */
#include "ViiROS.h"
#include "GPIO.h"

/*============================================================================*/
/*                       Thread Component Declaration                         */
/*============================================================================*/

/** Active_Thread[] contains started threads */
ViiROS_Thread *Active_Thread[33];

/**
*@brief Current thread (modified by PendSV -> volatile)
*
*@warning In the very first context switch == NULL!!! */
ViiROS_Thread * volatile ViiROS_current = NULL;

/** next thread (modified by PendSV -> volatile) */
ViiROS_Thread * volatile ViiROS_next = NULL;

/** Ready thread bitmask */
static volatile uint32_t ViiROS_readyMask = 0U; 

/** Blocked thread bitmask  */
static volatile uint32_t ViiROS_blockedMask = 0U;

/*============================================================================*/
/*                        Idle Thread Declaration                             */
/*============================================================================*/

ViiROS_Thread ViiROS_Idle; /**<TCB for Idle-Thread */
static uint32_t stack_ViiROS_Idle[40]; /**<stack for Idle-Thread */

/**
*@brief Idle-Thread-Handler
*
*@note  lowest priority - runs when readyMask == 0 
*@warning NEVER calls BlockTime()!!!
*/
static void ViiROS_onIdle(void)
{
  while(1){
    /* Only for visualization in Pulsview (Logic Analyzer) */
    uint32_t volatile i;
    for(i = 0; i < 1500; i++){
      GPIO_WritePin(GPIO_PORTF, Switch_2, 1U);
      GPIO_WritePin(GPIO_PORTF, Switch_2, 0U);
    }
    /* Only for visualization in Pulsview (Logic Analyzer) */
    
    
   //__WFI(); /**< Wait for interrupt and sleep until */
  }
}


/*============================================================================*/
/*                       Function Declarations                                */
/*============================================================================*/

/*--------------------------- lastInit() -------------------------------------*/
/**
*@brief Further initialization / code before ViiROS takes control
*
*@note  Defined as __weak so it won't trigger linker warning
*       Define code for lastInit in main.c 
*/
__weak void ViiROS_lastInit(void){};


/*----------------------------- Init() ---------------------------------------*/
/**
*@brief Initialization of PendSV, Idle-Thread, Current-Thread
*@note PendSV == lowest interrupt-priority
*@note Idle-Thread == lowest thread-priority
*@warning Current-Thread needs to be initialized to NULL!!!
*/
void ViiROS_Init()
{
  /* Set PendSV priority to the lowest */
  NVIC_SetPriority(PendSV_IRQn, 0xFFU);
  
  /* Start idle thread */
  ViiROS_ThreadStart(&ViiROS_Idle, 
                     ViiROS_onIdle, 
                     0U, 
                     stack_ViiROS_Idle, sizeof(stack_ViiROS_Idle));
  
  /* NULL for the very first context switch */
  ViiROS_current = NULL;
}

/*----------------------------- Scheduler ------------------------------------*/
/**
*@brief Preemptive Scheduler (Priority, Blocking)
*@note LOG2(readyMask) >> get highest priority in one operation
*@note Triggers PendSV
*/
void ViiROS_Scheduler(void)
{
  ViiROS_Thread *current = ViiROS_current; /**< Current-thread already known */
  ViiROS_Thread *next;
  
  if(ViiROS_readyMask == 0U) /**< no thread ready = idle */
  {
    next = Active_Thread[0]; /**< load idle thread */
  }
  else 
  {
    uint32_t highPrio = LOG2(ViiROS_readyMask); /**< highest ready prio */
    next = Active_Thread[highPrio]; /**< load highest ready Thread into next */
  }
  
  if(current != next) /**< update and trigger PendSV only when thread changed */
  {
    ViiROS_next = next; /**< set the next thread to run */
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; /**<trigger PendSV for context switch */
  } 
}

/*------------------------------ Run() ---------------------------------------*/

/**
*@brief Performs last configuration/initialization and gives control to kernel
*@note  lastInit() can perform further configuration/initialization 
*@warning  After the Scheduler call it never returns to this function
*@warning  If(Current-Thread != 0) => Idle-Thread returns to main 
*          -> Idle_thread->sp = msp after contex switch  -> System failure 
*/
void ViiROS_Run(void)
{
  ViiROS_lastInit(); /**<Add here individual code e. g. Switch interrupt,...  */
  
  /* Start Critical Section */
  __disable_irq();
  
  /* Give up control to ViiROS */
  ViiROS_Scheduler();
  
  /* End Critical Section */
  __enable_irq();
  
  /* following code will never execute - just for debugging purpose */
 while(1)
 {   
   GPIO_WritePin(GPIO_PORTF, RED_LED, 1U);
    
   for(uint32_t i = 0; i < 100000; i ++){}
    
    GPIO_WritePin(GPIO_PORTF, RED_LED, 0U);
    for(uint32_t i = 0; i < 100000; i ++){}
    
    GPIO_WritePin(GPIO_PORTF, GREEN_LED, 1U);
    for(uint32_t i = 0; i < 100000; i ++){}
    
    GPIO_WritePin(GPIO_PORTF, GREEN_LED, 0U);
    for(uint32_t i = 0; i < 100000; i ++){}
    
    GPIO_WritePin(GPIO_PORTF, BLUE_LED, 1U);
    for(uint32_t i = 0; i < 100000; i ++){}
    
    GPIO_WritePin(GPIO_PORTF, BLUE_LED, 0U);
    for(uint32_t i = 0; i < 100000; i ++){}
 }
}

/*----------------------------- BlockTime() ----------------------------------*/

/**
*@brief Blocks thread for X ms
*@note Sets/Clears readyMask/blockedMask, Sets thread->blocktime
*@param time = time thread spends in blocked state in ms
*@warning Call in idle-thread is forbidden!
*/
void ViiROS_BlockTime(uint32_t time)
{  
  uint32_t current_bit; /**< local Bit-Variable for local thread management */

  __disable_irq(); /**< Start Critical Section */
  
  if(ViiROS_current != Active_Thread[0]) /**< safety for Idle-Thead */
  {
    /* Mask[bit] => bit = priority - 1 */ 
    current_bit = 1U << (ViiROS_current->priority - 1U);
    
    ViiROS_current->blocktime = time; /**< set blocktime */
    ViiROS_readyMask &= ~current_bit; /**< clear bit in readyMask */
    ViiROS_blockedMask |= current_bit; /**< set bit in blockedMask */
    
    ViiROS_Scheduler(); /**< call Scheduler for update */
  }
     
  __enable_irq(); /**< End Critical Section */
}

/*----------------------------- blockWatch() ---------------------------------*/

/**
*@brief Keeps track of blocktime / updates readyMask/blockedMask
*@note Local copy of blockedMask to iterate through all blocked threads
*@note Iterates only over n blocked - very efficient and fast
*/
void ViiROS_BlockWatch(void)
{
  if(ViiROS_blockedMask != 0U)/**< runs only if blocked threads exist */
  {
    uint32_t watchSet = ViiROS_blockedMask; /**< local copy to work with */
    
    /* repeat as long as at least one blocked thread remains */
    while (watchSet != 0U)
    {
      uint32_t prio = LOG2(watchSet); /**< get next blocked thread */
      
      /* Pointer to the blocked Thread-TCB */
      ViiROS_Thread *thread = Active_Thread[prio]; 
      
      --thread->blocktime; /**< decrement blockedtime of selected thread*/
      
      /** index = prio - 1 - array[index] */
      uint32_t watchBit = 1U << (prio - 1U); 
      watchSet &= ~watchBit; /**< clear selected thread from local copy */
      
      /** when reaching 0 manage the ready- and blockedMask */
      if(thread->blocktime == 0U)
      {
        ViiROS_blockedMask &= ~watchBit; /**< clear blocked thread */
        ViiROS_readyMask |= watchBit; /**< set thread as ready */
      }
    }
  }
}

/*----------------------------- ThreadStart() --------------------------------*/

/* ViiROS Thread start */
/**
*@brief Creates threads including fabricated initial hard- and software stack
*
*@note Stack-Size in Bytes (Example: 80 Words = 320 Bytes : 1 Word = 4 Bytes)
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
*@note  LR = 0xFFFFFFFD // 0xFFFFFFED (FPU)
*       Special value for CPU to know to return from interrupt to thread.
*/
void ViiROS_ThreadStart(
     ViiROS_Thread *me, 
     ViiROS_ThreadHandler thread_Handler, 
     uint8_t priority, 
     void *stk_Storage, uint32_t stk_Size)
{
  /** Check if priority is in range and not already taken */
  if (priority <= 32 &&
      Active_Thread[priority] == NULL){
      
      /**
        Setup the sp to the top of the stack of the thread.
        AAPCS (Procedure Call Standard): 8-Byte-alignment on ARM Cortex-M4    
      */
      uint32_t *sp = (uint32_t *)((uint32_t)stk_Storage + stk_Size);
      sp = (uint32_t *)((uint32_t)sp & ~0x7);  
 
      
      /** hardware stack frame - caller saved */
      *--sp = (1U << 24);        /**< xPSR (Thumb-Bit = 1 if 0 = HardFault)*/
      *--sp = ((uint32_t)thread_Handler | 1U); /* PC – entry point */
      *--sp = 0xFFFFFFFD;        /**< LR = 0xFFFFFFFD => EXC_RETURN */
      *--sp = 0xCAFECAFE;        /**< R12 */
      *--sp = 0xCAFECAFE;        /**< R3 */
      *--sp = 0xCAFECAFE;        /**< R2 */
      *--sp = 0xCAFECAFE;        /**< R1 */
      *--sp = 0xCAFECAFE;        /**< R0 */  
      /* software stack frame - calle saved  */
      *--sp = 0xCAFEBABE;        /**< R11 */
      *--sp = 0xCAFEBABE;        /**< R10 */
      *--sp = 0xCAFEBABE;        /**< R9 */
      *--sp = 0xCAFEBABE;        /**< R8 */
      *--sp = 0xCAFEBABE;        /**< R7 */
      *--sp = 0xCAFEBABE;        /**< R6 */
      *--sp = 0xCAFEBABE;        /**< R5 */
      *--sp = 0xCAFEBABE;        /**< R4 */ 
      
      me->sp = sp; /**< set sp of thread to the calculated calue for R4 */
      
      /* End of stack - lowest address */
      uint32_t *stack_limit = (uint32_t *)stk_Storage;
      
      /* Prefil the unused stack */
      for(uint32_t *i = sp; i > stack_limit; i--){
        *--sp = 0xDEADBABE;
      }
 
      /** Register the thread in the active thread array */
      Active_Thread[priority] = me;
      me->priority = priority; /**< save desired priority */
      
      /** mark thread as ready by setting the corresponding bit in readyMask  */
      if (priority > 0)
      {
        uint32_t bit =  (1U << (priority - 1U));
        ViiROS_readyMask |= bit;
      }
  }    
} 

/*----------------------------- PendSV_Handler() -----------------------------*/
/**
*@brief Performs the context switch
*@note  Code can only be in assembly
*@note  Most of the assembly code is copied from disassembler
*@note  1. Write C-code 2. compiler generates asm-code 3. copy from disassembler
*       4. Add: MOV R1, SP; POP/PUSH {r4-r11};  
*@code 
*       void PendSV_Handler()
*       { 
*         void *sp;
*          
*        __disable_irq();
*         
*         if(ViiROS_current != (ViiROS_Thread *)0)
*         {
*           //push R4 - R11
*           ViiROS_current->sp = sp;  
*         }
*          
*         sp = ViiROS_next->sp;
*        
*         ViiROS_current = ViiROS_next;
*          
*           //pop R4 - R11 
*          __enable_irq(); 
*        }
*@endcode
*@note  Reseult: Code from disassembler
*    __disable_irq();
*                 PendSV_Handler:
*        0x1e6: 0xb672         CPSID     i
*    if (ViiROS_current != 0)
*        0x1e8: 0x4a0b         LDR.N     R2, ??DataTable6_5      ; ViiROS_current
*        0x1ea: 0x6810         LDR       R0, [R2]
*        0x1ec: 0x2800         CMP       R0, #0
*        0x1ee: 0xd001         BEQ.N     ??PendSV_Handler_0      ; 0x1f4
*        ViiROS_current->sp = sp;
*        0x1f0: 0x6810         LDR       R0, [R2]
*       0x1f2: 0x6001         STR       R1, [R0]
*    sp = ViiROS_next->sp;
*                 ??PendSV_Handler_0:
*        0x1f4: 0x490b         LDR.N     R1, ??DataTable6_8      ; ViiROS_next
*        0x1f6: 0x6808         LDR       R0, [R1]
*        0x1f8: 0x6800         LDR       R0, [R0]
*   ViiROS_current = ViiROS_next;
*        0x1fa: 0x6808         LDR       R0, [R1]
*        0x1fc: 0x6010         STR       R0, [R2]
*    __enable_irq();
*        0x1fe: 0xb662         CPSIE     i
*
*@warning Code from Disassembly need manuel modification!!!
*@warning __stackless is IAR specific -> __attribute__((naked)) GCC / Keil MDK
*/ 
__stackless void PendSV_Handler(void)
{
  /* __asm - CMSIS = Code in Assembler in IAR / KEIL MDK // __asm__ in GCC */
  __asm volatile(
  
        /* __disable_irq(); start Critical Section */
       " CPSID     i                    \n"
       
       
       /* if (ViiROS_current != 0) */
       " LDR.N     R2, =ViiROS_current   \n"    // R2 = &current 
       " LDR       R0, [R2]              \n"    // R0 = current-TCB (Value of R2)
       " CBZ       R0, PendSV_first_run  \n"    // R0 == 0? -> jump to first_run
         
       /* #### Save current-thread ###### */
       /* save R4 - R11 on stack */  
       " MRS       R1, PSP               \n"    // R1 = PSP
         
       /* Store Multiple, Decrement Before */
       " STMDB     R1!, {R4-R11}         \n"    // R1 = new PSP at R4
       /* PSP = ViiROS_next->sp; */
       " STR       R1, [R0]              \n"    // TCB->sp = R1
         
       /*##### Load next-thread ##########*/
       "PendSV_restore:                  \n"
       " LDR       R0, =ViiROS_next      \n"     // R0 = next
       " LDR       R1, [R0]              \n"     // R1 = next-TCB (Value at R0)
       " LDR       R0, [R1]              \n"     // R0 = next-TCB->sp
       /* Load multiple, increment after */   
       " LDMIA     R0!, {R4-R11}         \n"     // Load R4 - R11, new sp at R0
       " MSR       PSP, R0               \n"     // PSP = R0 (SP)
        
         
        /* ViiROS_current = ViiROS_next; update current-thread */
       " STR       R1, [R2]             \n"     //  R2 = &ViiROS_current 
                                                // [R2] = Value at &Viiros_current
         
        /* __enable_irq(); end Critical Section */
       " CPSIE     i                    \n"     // enable interrupt
       " BX        LR                   \n"     // Jump with EXC_RETURN value

       /*#### FIRST pendSV RUN ######### */  
       "PendSV_first_run:               \n"
       
       " MOV       R3, #0x02            \n" 
       " MSR       CONTROL, R3          \n" // set CONROLL = 0x02
       " ISB                            \n" // make sure changes took place
         
       /* EXC_RETURN 0xFFFFFFFD = return to Thread Mode using PSP */
       " LDR       LR, =0xFFFFFFFD      \n" // bug fix! wihtout it interrupt returns to main
       " B         PendSV_restore       \n" 
  );
}
