/*
Copyright (C) 2015 Alexander Sadakov <al.sadakov_dog_gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "stm8s.h"
#include "stm8s_gpio.h"

#include <stdio.h>


#include "hw.h"
#include "uart.h"


#define get_flag(flagvar) (flags>>flagvar) & 1
#define set_flag(flagvar) disableInterrupts(); flags |= (1<<flagvar); enableInterrupts()
#define reset_flag(flagvar) disableInterrupts(); flags &= ~(1<<flagvar); enableInterrupts()

#define RES_BUF_LEN 32
#define SIG_CHECK_TICKS 16

#define ALARM_LEN 4
//#define SIG_SUM_TRESHOLD 10
#define SIG_SUM_TRESHOLD_LOW 4
#define SIG_SUM_TRESHOLD_HIGH 10

#define HALT_COUNTDOWN_TICKS 16

#define BTN_HALT_TICKS 16

typedef enum {
    MODE_LOW_TRESHOLD,
    MODE_HIGH_TRESHOLD,
    MODE_BEACON,
    MODES_NUM
} MODES;

typedef enum {
    FLAG_END_ADC_CONV,			//end sig measure
    FLAG_TIMER_TICK,
    FLAG_DEVICE_HALTED,
    FLAG_BUTTON_PRESSED,
    FLAG_VCC_MEASURE
    
} FLAGS;


void delay(uint32_t t);
