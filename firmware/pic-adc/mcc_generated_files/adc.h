#ifndef _ADC_H
#define _ADC_H


#include <xc.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus  // Provide C++ Compatibility
extern "C" {
#endif


typedef enum {
    channel_AN2 = 2,
    channel_AN3 = 3,
    channel_AN6 = 6,
    channel_AN7 = 7
} adc_channel_t;


typedef uint16_t adc_result_t;


void ADC_Initialize(void);
void ADC_ISR(void);


#ifdef __cplusplus  // Provide C++ Compatibility
}
#endif


#endif

