#ifndef _TIMER_H
#define _TIMER_H

#include "sys.h"
#include "adc.h"
#include "stm32f10x_tim.h"

#define true 1
#define false 0

extern u8 BPM;                   			 // used to hold the pulse rate
extern short temperature; 
  
void TIM3_Int_Init(u16 arr,u16 psc);

#endif
