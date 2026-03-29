#include "TM4C123GH6PM.h"       // TI - MCU - CMSIS - Hardware 
#include "system_TM4C123.h"     // TI - Board - CMSIS - start up code
#include "core_cm4.h"           // CMSIS Header __disable_irq(), ...
#include "SysTick.h"
#include "GPIO.h"
#include "ViiROS.h"

/*============================================================================*/
/*                             Thread Data                                    */
/*============================================================================*/

ViiROS_Thread Red; // Thread Red TCB
static uint32_t stack_Red[80]; // Stack => nicht zu klein wählen

/* Handlers are programmed "busy" for visualization in Pulsview (Logic Analyzer) */ 
void Red_t(void)
{
  while(1){
    /* toggle red led */
    uint32_t volatile i;
    for(i = 0; i < 500; i++){
      GPIO_WritePin(GPIO_PORTF, RED_LED, ON);
      GPIO_WritePin(GPIO_PORTF, RED_LED, OFF);
    } 
    /* Block for 12 ms */
    ViiROS_BlockTime(12U);
  }

}

ViiROS_Thread Blue;
static uint32_t stack_Blue[80];

void Blue_t(void)
{
  while(1){
    /* toggle blue led */
    uint32_t volatile i;
    for(i = 0; i < 1500; i++){
      GPIO_WritePin(GPIO_PORTF, BLUE_LED, ON);
      GPIO_WritePin(GPIO_PORTF, BLUE_LED, OFF);
    }
    /* Block for 16 ms */   
    ViiROS_BlockTime(16U);
  }
}

ViiROS_Thread Green;
static uint32_t stack_Green[80];

void Green_t(void)
{
  while(1)
  {
    /* toggle green led */
     uint32_t volatile i;
    for(i = 0; i < 5000; i++){
      GPIO_WritePin(GPIO_PORTF, GREEN_LED, ON);
      GPIO_WritePin(GPIO_PORTF, GREEN_LED, OFF);
    }
    /* Block for 21 ms */
    ViiROS_BlockTime(21U); 
  }
}


/*============================================================================*/
/*                               Main                                         */
/*============================================================================*/

int main()
{
    /* Update the System Clock */
    SystemCoreClockUpdate(); 
    
    /* defensive initialization by disabling interrupts globally */
    __disable_irq();
    
    /* SysTick - Initialization */
    SysTick_Init();
    
    /* GPIO - Initialization */
    GPIO_EnablePort(GPIO_PORTF); /* Enable clock for Port F */
    
    /* Configure pins as outputs */
    GPIO_ConfigureOutput(GPIO_PORTF, Switch_1);
    GPIO_ConfigureOutput(GPIO_PORTF, Switch_2); 
    GPIO_ConfigureOutput(GPIO_PORTF, RED_LED);   
    GPIO_ConfigureOutput(GPIO_PORTF, BLUE_LED); 
    GPIO_ConfigureOutput(GPIO_PORTF, GREEN_LED);  

    /* defined state == 0 */
    GPIO_WritePin(GPIO_PORTF, Switch_1, OFF);
    GPIO_WritePin(GPIO_PORTF, Switch_2, OFF);
    GPIO_WritePin(GPIO_PORTF, RED_LED, OFF); 
    GPIO_WritePin(GPIO_PORTF, BLUE_LED, OFF);  
    GPIO_WritePin(GPIO_PORTF, GREEN_LED, OFF); 
    
    /* ViiROS - Initialization */
    ViiROS_Init();
    
    /* start user threads*/
    ViiROS_ThreadStart(&Red,
                       Red_t,
                       3U,
                       stack_Red, sizeof(stack_Red));
    
    ViiROS_ThreadStart(&Blue,
                       Blue_t,
                       2U,
                       stack_Blue, sizeof(stack_Blue));
    
    ViiROS_ThreadStart(&Green,
                       Green_t,
                       1U,
                       stack_Green, sizeof(stack_Green));
    
    
    /* enable interrups after initialiaztion */
    __enable_irq();
    
    /* give up controll to ViiROS -> MSP --> PSP */
    ViiROS_Run();

}