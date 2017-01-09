/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup cc2538
 * @{
 *
 * \defgroup cc2538-clock cc2538 Clock
 *
 * Implementation of the clock module for the cc2538
 *
 * To implement the clock functionality, we use the SysTick peripheral on the
 * cortex-M3. We run the system clock at a configurable speed and set the 
 * SysTick to give us 128 interrupts / sec. However, the Sleep Timer counter 
 * value is used for the number of elapsed ticks in order to avoid a 
 * significant time drift caused by PM1/2. Contrary to the Sleep Timer, the 
 * SysTick peripheral is indeed frozen during PM1/2, so adjusting upon wake-up 
 * a tick counter based on this peripheral would hardly be accurate.
 * @{
 *
 * \file
 * Clock driver implementation for the TI cc2538
 */
#include "contiki.h"
#include "cc2538_cm3.h"
#include "reg.h"
#include "cpu.h"
#include "dev/gptimer.h"
#include "dev/sys-ctrl.h"

#include "sys/energest.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define RTIMER_CLOCK_TICK_RATIO (RTIMER_SECOND / CLOCK_SECOND)

/* Prescaler for GPT0:Timer A used for clock_delay_usec(). */
#if SYS_CTRL_SYS_CLOCK < SYS_CTRL_1MHZ
#error System clock speeds below 1MHz are not supported
#endif
#define PRESCALER_VALUE         (SYS_CTRL_SYS_CLOCK / SYS_CTRL_1MHZ - 1)

/* Period of the SysTick counter expressed as a number of ticks */
#if SYS_CTRL_SYS_CLOCK % CLOCK_SECOND
/* Too low clock speeds will lead to reduced accurracy */
#error System clock speed too slow for CLOCK_SECOND, accuracy reduced
#endif
#define SYSTICK_PERIOD          (SYS_CTRL_SYS_CLOCK / CLOCK_SECOND)

static volatile rtimer_clock_t rt_last;
static volatile clock_time_t current_time;
static unsigned long current_time_epoch;
/*---------------------------------------------------------------------------*/
/**
 * \brief Arch-specific implementation of clock_init for the cc2538
 *
 * We initialise the SysTick to fire 128 interrupts per second, giving us a
 * value of 128 for CLOCK_SECOND
 *
 * We also initialise GPT0:Timer A, which is used by clock_delay_usec().
 * We use 16-bit range (individual), count-down, one-shot, no interrupts.
 * The prescaler is computed according to the system clock in order to get 1
 * tick per usec.
 */
void
clock_init(void)
{
  SysTick_Config(SYSTICK_PERIOD);

  /*
   * Remove the clock gate to enable GPT0 and then initialise it
   * We only use GPT0 for clock_delay_usec. We initialise it here so we can
   * have it ready when it's needed
   */
  REG(SYS_CTRL_RCGCGPT) |= SYS_CTRL_RCGCGPT_GPT0;

  /* Make sure GPT0 is off */
  REG(GPT_0_BASE + GPTIMER_CTL) = 0;

  /* 16-bit */
  REG(GPT_0_BASE + GPTIMER_CFG) = 0x04;

  /* One-Shot, Count Down, No Interrupts */
  REG(GPT_0_BASE + GPTIMER_TAMR) = GPTIMER_TAMR_TAMR_ONE_SHOT;

  /* Prescale depending on system clock used */
  REG(GPT_0_BASE + GPTIMER_TAPR) = PRESCALER_VALUE;

  rt_last = RTIMER_NOW();
}
/*---------------------------------------------------------------------------*/
CCIF clock_time_t
clock_time(void)
{
  clock_time_t r1, r2;
  do {
    r1 = current_time;
    r2 = current_time;
  } while(r1 != r2);
  return r1;
}
/*---------------------------------------------------------------------------*/
void
clock_set_seconds(unsigned long sec)
{
  current_time_epoch = sec - clock_seconds();
}
/*---------------------------------------------------------------------------*/
CCIF unsigned long
clock_seconds(void)
{
  return ((unsigned long)(clock_time() / CLOCK_SECOND)) + current_time_epoch;
}
/*---------------------------------------------------------------------------*/
void
clock_wait(clock_time_t i)
{
  clock_time_t start;

  start = clock_time();
  while(clock_time() - start < (clock_time_t)i);
}
/*---------------------------------------------------------------------------*/
/*
 * Arch-specific implementation of clock_delay_usec for the cc2538
 *
 * See clock_init() for GPT0 Timer A's configuration
 */
void
clock_delay_usec(uint16_t dt)
{
  REG(GPT_0_BASE + GPTIMER_TAILR) = dt;
  REG(GPT_0_BASE + GPTIMER_CTL) |= GPTIMER_CTL_TAEN;

  /* One-Shot mode: TAEN will be cleared when the timer reaches 0 */
  while(REG(GPT_0_BASE + GPTIMER_CTL) & GPTIMER_CTL_TAEN);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Obsolete delay function but we implement it here since some code
 * still uses it
 */
void
clock_delay(unsigned int i)
{
  clock_delay_usec(i);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Adjust the clock following missed SysTick ISRs
 *
 * This function is useful when coming out of PM1/2, during which the system
 * clock is stopped. We adjust the clock counters like after any SysTick ISR.
 *
 * \note This function is only meant to be used by lpm_exit(). Applications
 * should really avoid calling this
 */
void
clock_adjust(rtimer_clock_t ticks)
{
  uint64_t adjustment;

  /*
   * We need to convert sleep duration from rtimer ticks to clock
   * ticks, adjustment/(RTIMER_SECOND/CLOCK_SECOND)
   */
  adjustment = ticks;
  adjustment = adjustment * CLOCK_SECOND;
  adjustment = adjustment / RTIMER_SECOND;

  /* Halt the SysTick while adjusting */
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

  current_time += (clock_time_t)adjustment;

  /* Re-Start the SysTick */
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

  /*
   * Inform the etimer library that the system clock has changed and that an
   * etimer might have expired.
   */
  if(etimer_pending()) {
    etimer_request_poll();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief The clock Interrupt Service Routine
 *
 * It polls the etimer process if an etimer has expired. It also updates the
 * software clock tick and seconds counter.
 */
void
clock_isr(void)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  current_time++;

  /*
   * Inform the etimer library that the system clock has changed and that an
   * etimer might have expired.
   */
  if(etimer_pending()) {
    etimer_request_poll();
  }

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/

/**
 * @}
 * @}
 */
