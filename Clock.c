/**
 * @file Clock.c
 * @brief System clock configuration module
 * 
 * This module handles the system clock initialization and provides
 * clock-related utility functions. Currently configured for the
 * default 16 MHz system clock after reset.
 * 
 * The TM4C123GH6PM starts up with the main oscillator (MOSC) configured
 * to use the 16 MHz crystal, providing a 16 MHz system clock without
 * any additional configuration.
 */

#include "Clock.h"



/**
 * @brief Calculate SysTick reload value for 1ms period
 * 
 * Computes the value to load into the SysTick reload register to achieve
 * a 1ms interrupt period. The calculation is:
 * 
 * SysTick_Reload = (System_Core_Clock / 1000) - 1
 * 
 * @warning This function assumes System_Core_Clock is correctly set
 * @see SysTick_Init() uses this value to configure the timer
 */
uint32_t SysTick_Reload_Value(void)
{
    
    uint32_t SysTick_Reload = (SystemCoreClock / 1000U) - 1U;
    
    return SysTick_Reload;
}