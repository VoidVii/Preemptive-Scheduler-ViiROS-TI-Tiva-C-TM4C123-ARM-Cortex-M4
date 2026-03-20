#include "TM4C123GH6PM.h"       // TI - MCU - CMSIS 
#include "system_TM4C123.h"     // TI - Board - CMSIS 
#include "core_cm4.h"           // CMSIS Header
#include "SysTick.h"
#include "GPIO.h"
#include "ViiROS.h"



/*============================================================================*/
/*                             Thread Data                                    */
/*============================================================================*/



ViiROS_Thread Red;
static uint32_t stack_Red[80];

void Red_t(void)
{
  while(1){
    GPIO_WritePin(GPIO_PORTF, 1U, 1U);
    ViiROS_BlockTime(250U);
    GPIO_WritePin(GPIO_PORTF, 1U, 0U);
    ViiROS_BlockTime(500U);
  }
}

ViiROS_Thread Green;
static uint32_t stack_Green[80];

void Green_t(void)
{
  while(1){
   GPIO_WritePin(GPIO_PORTF, 3U, 1U);  // Blaue LED an
   ViiROS_BlockTime(400U);
   GPIO_WritePin(GPIO_PORTF, 3U, 0U);  // Blaue LED aus
   ViiROS_BlockTime(200U);

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
    GPIO_EnablePort(GPIO_PORTF);      // Enable clock for Port F

    GPIO_ConfigureOutput(GPIO_PORTF, 1U);  // Red LED as output
    GPIO_ConfigureOutput(GPIO_PORTF, 3U);  // Blue LED as output
    
    GPIO_WritePin(5U, 1U, 0U);
    GPIO_WritePin(5U, 3U, 0U);
    
    /* ViiROS - Initialization */
    ViiROS_Init();
    
    ViiROS_ThreadStart(&Red,
                       Red_t,
                       32U,
                       stack_Red, sizeof(stack_Red));
    
    ViiROS_ThreadStart(&Green,
                       Green_t,
                       7U,
                       stack_Green, sizeof(stack_Green));

    /* enable interrups after initialiaztion */
    __enable_irq();
    
    /* give up controll to ViiROS -> MSP --> PSP */
    ViiROS_Run();

}