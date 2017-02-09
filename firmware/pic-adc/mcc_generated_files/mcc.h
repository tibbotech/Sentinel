#ifndef MCC_H
#define	MCC_H


#include <xc.h>
#include "pin_manager.h"
#include <stdint.h>
#include <stdbool.h>
#include "interrupt_manager.h"
#include "i2c.h"
#include "adc.h"


#define _XTAL_FREQ  32000000


void SYSTEM_Initialize(void);
void OSCILLATOR_Initialize(void);
void WDT_Initialize(void);


#endif