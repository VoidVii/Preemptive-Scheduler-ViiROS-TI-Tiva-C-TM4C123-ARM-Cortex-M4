#include "TM4C123GH6PM.h"       // TI - MCU - CMSIS 
#include "system_TM4C123.h"     // TI - Board - CMSIS 
#include "core_cm4.h"           // CMSIS Header
#include "SysTick.h"
#include "GPIO.h"
#include "Clock.h"
#include "LED.h"
#include "ViiROS.h"
#include "Debounce_Switch.h"
#include <stdint.h>

/************************** Functions for Object-Handling ********************/
/**
 * @brief Write function for red LED (Port F, Pin 1)
 * @param Value 1 = ON, 0 = OFF (according to activeHigh setting)
 */
void RED_LED_ON(uint8_t Value){
    GPIO_WritePin(GPIO_PORTF, 1U, Value);
}

/**
 * @brief Write function for blue LED (Port F, Pin 2)
 * @param Value 1 = ON, 0 = OFF (according to activeHigh setting)
 */
void BLUE_LED_ON(uint8_t Value){
    GPIO_WritePin(GPIO_PORTF, 2U, Value);
}


ViiROS_Thread Red;
uint32_t stack_Red[40];

void Red_t(void)
{
  RED_LED_ON(1U);
  ViiROS_BlockTime(250U);
  RED_LED_ON(0U);
  ViiROS_BlockTime(100);
}

ViiROS_Thread Blue;
uint32_t stack_Blue[40];

void Blue_t(void)
{
  BLUE_LED_ON(1U);
  ViiROS_BlockTime(400U);
  BLUE_LED_ON(0U);
  ViiROS_BlockTime(150);
}

/****************************** Build Objects  ********************************/
/* LED Objects - each with its own write function and state */
LED_t red_LED = {
    .write = RED_LED_ON,        // Function to write to this specific LED
    .currentState = 0U,         // Current state: 0 = OFF, 1 = ON
    .activeHigh = 1U,           // 1 = HIGH turns LED on, 0 = LOW turns LED on
};

LED_t blue_LED = {
    .write = BLUE_LED_ON,
    .currentState = 0U,
    .activeHigh = 1U,
};




int main()
{
    /* Update the System Clock*/
    SystemCoreClockUpdate(); 
    
    /* defensive initialization by disabling interrupts globally */
    __disable_irq();
    
    // Initialize system modules
    SysTick_Init();
   
    GPIO_EnablePort(GPIO_PORTF);      // Enable clock for Port F
    
    // Configure GPIO pins

    GPIO_ConfigureOutput(GPIO_PORTF, 1U);  // Red LED as output
    GPIO_ConfigureOutput(GPIO_PORTF, 2U);  // Blue LED as output
      
    // Initialize all LEDs to OFF state
    LED_off(&red_LED);
    LED_off(&blue_LED);
    
    
    
    /* enable interrupts globally after initialization  */
    __enable_irq();
   
    
    // Main super loop - runs forever
    while(1)
    {
       
    }
}