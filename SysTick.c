/**
 * @file SysTick.c
 * @brief SysTick timer driver for system timekeeping
 * 
 * This module initializes and manages the SysTick timer to provide
 * a 1ms system tick. It maintains a 32-bit counter that increments
 * every millisecond, allowing for accurate time measurements and delays.
 * 
 * The SysTick timer is a 24-bit decrementing counter that generates
 * an interrupt when it reaches zero, automatically reloading from
 * the RELOAD register.
 */
#include "TM4C123GH6PM.h"
#include "core_cm4.h"
#include "SysTick.h"
#include "system_TM4C123.h"
#include "ViiROS.h"

uint32_t SysTick_Reload_Value(void)
{
    /**
     * Calculate reload value for 1ms period
     * 
     * Formula: Reload = (Clock_Hz / 1000) - 1
     * 
     * Example with 16 MHz:
     * (16,000,000 / 1000) - 1 = 16,000 - 1 = 15,999
     * 
     * The timer counts 16,000 cycles (from 15999 down to 0) at 16 MHz,
     * taking exactly 1ms (16,000 / 16,000,000 = 0.001 seconds).
     */
    uint32_t SysTick_Reload = (SystemCoreClock / 1000U) - 1U;
    
    return SysTick_Reload;
}

/**
 * @brief Initialize the SysTick timer for 1ms interrupts
 * 
 * Configures the SysTick timer to generate an interrupt every 1ms.
 * The reload value is calculated based on the current system clock frequency.
 * 
 * SysTick timer details:
 * - 24-bit decrementing counter
 * - Counts down from RELOAD value to 0
 * - Generates interrupt on reaching 0
 * - Automatically reloads from RELOAD register
 * - Stops when processor is halted for debugging
 * 
 * @note Must be called before using any time-related functions
 * @note System clock must be configured before calling this
 * 
 * @see SysTick_Reload_Value() (system_TM4C123.c) Calculates reload value based on clock
 * @see GetTickCounter() for reading the current tick count
 */
void SysTick_Init(void)
{
    /* Disable SysTick during configuration to prevent unexpected interrupts */
    SysTick->CTRL = 0U;
    
    /* Clear current value (write any value to reset) */
    SysTick->VAL = 0U;
    
    SysTick->LOAD = SysTick_Reload_Value();
    
    /**
     * Configure SysTick control register:
     * Bit 0: Enable counter (ENABLE)
     * Bit 1: Enable interrupt (INTEN)
     * Bit 2: Clock source - System clock (CLK_SRC)
     */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

/**
 * @brief 32-bit system tick counter
 * 
 * This static volatile variable holds the number of milliseconds
 * since system startup. It's incremented every 1ms in the SysTick
 * interrupt handler.
 *
 * - volatile prevents compiler optimizations (changed in interrupt)
 * - static limits scope to this file
 */
static volatile uint32_t TickCounter;

/**
 * @brief SysTick interrupt handler
 * 
 * Called automatically every 1ms when the SysTick timer reaches zero.
 * Increments the global TickCounter to maintain system time.
 * 
 * This handler should be kept as short as possible since it runs
 * at interrupt context. No complex operations should be performed here.
 * 
 * @note Runs at interrupt priority (typically highest)
 * @warning Keep this function minimal to avoid interrupt latency
 */
void SysTick_Handler(void)
{
    TickCounter++; /* As long as only SysTick has access its safe */
    
    __disable_irq();
    
    ViiROS_BlockWatch();
    ViiROS_Scheduler();
    
    __enable_irq();
}

/**
 * @brief Get the current system tick count
 */
uint32_t GetTickCounter(void)
{
    return TickCounter;
}