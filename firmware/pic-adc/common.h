#ifndef COMMON_H
#define	COMMON_H


#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "mcc_generated_files/adc.h"


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _adc_ch {
    uint16_t count;
    adc_result_t min;
    adc_result_t max;
    adc_result_t buf[5];
    uint16_t res;
} adc_ch_t;


extern volatile adc_ch_t adc_channels[4];


#ifdef	__cplusplus
}
#endif


#endif	/* COMMON_H */

