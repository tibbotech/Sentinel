#include <xc.h>
#include <pic16f1824.h>
#include "mcc_generated_files/mcc.h"
#include "common.h"


#pragma config IDLOC0 = 0x41        // 'A'
#pragma config IDLOC1 = 0x31        // '1'
#pragma config IDLOC2 = 1           // v1.0
#pragma config IDLOC3 = 0


#define SAMPLES_MAX 250     
#define USE_BLINKER


volatile adc_ch_t adc_channels[4];  
volatile uint8_t adc_ch;
#ifdef USE_BLINKER
static uint8_t blink_ctr;
#endif


#ifdef USE_BLINKER
static void 
blink()
{
    if(++blink_ctr == 4) {
        PORTAbits.RA5 = PORTAbits.RA5 ^ 1;
        blink_ctr = 0;
    }
}
#endif


#if 0
void 
ADC_ISR(void)
{
    // Clear the ADC interrupt flag
    PIR1bits.ADIF = 0;
}
#endif


void
PIN_MANAGER_Initialize(void)
{
    /**
    ANSELx registers
    */   
    ANSELA = 0x17;
    ANSELC = 0x0C;

    /**
    APFCONx registers
    */
    APFCON0 = 0x00;
    APFCON1 = 0x00;

    /**
    LATx registers
    */   
    LATA = 0x00;    
    LATC = 0x00;    

    /**
    TRISx registers
    */    
    TRISA = 0x3F;
    TRISC = 0x3F;

    /**
    WPUx registers
    */ 
    WPUA = 0x2B;
    WPUC = 0x33;
    
    OPTION_REGbits.nWPUEN = 0;

}       


void 
ADC_Initialize(void)
{
    // set the ADC to the options selected in the User Interface
    
    // GO_nDONE stop; ADON enabled; CHS AN0; 
    ADCON0 = 0x09;
    
    // ADFM right; ADNREF VSS; ADPREF VDD; ADCS FOSC/4; 
    ADCON1 = 0xF0;
    
    // ADRESL 0; 
    ADRESL = 0x00;
    
    // ADRESH 0; 
    ADRESH = 0x00;
    
    // Enabling ADC interrupt.
    PIE1bits.ADIE = 0;
    
    ADCON0bits.ADON = 1;
}


void interrupt 
INTERRUPT_InterruptManager (void)
{
    if(INTCONbits.PEIE == 1 && PIE1bits.ADIE == 1 && PIR1bits.ADIF == 1)
    {
        //ADC_ISR();
    }
    else if(INTCONbits.PEIE == 1 && PIE1bits.SSP1IE == 1 && PIR1bits.SSP1IF == 1)
    {
        I2C_ISR();
    }
    else if(INTCONbits.PEIE == 1 && PIE1bits.TMR1IE == 1 && PIR1bits.TMR1IF == 1)
    {
        //TMR1_ISR();
    }
    else
    {
        //Unhandled Interrupt
    }
}


#ifdef USE_BLINKER
void
LED_Initialize()
{
    TRISAbits.TRISA5 = 0;
    PORTAbits.RA5 = 1;
}
#endif


static void
adc_start_measure()
{
    //ADCON0bits.ADON = 0;  
    switch(adc_ch) {
        case 0:
            ADCON0bits.CHS = channel_AN3;
            break;

        case 1:
            ADCON0bits.CHS = channel_AN7;
            break;

        case 2:
            ADCON0bits.CHS = channel_AN2;
            break;

        case 3:
            ADCON0bits.CHS = channel_AN6;
            break;

        default:
            return;
    }
    //ADCON0bits.ADON = 1;  
    //ADRESH = 0;
    //ADRESL = 0;
    ADCON0bits.GO_nDONE = 1;
}


void
main(void)
{
    uint16_t smpl_val;
    uint32_t result; 
    adc_ch_t volatile *pchn;
    float tmp;
    uint8_t i;
    
    SYSTEM_Initialize();
#ifdef USE_BLINKER    
    LED_Initialize();
#endif
    
    for(i=0; i<4; ++i) {
        adc_channels[i].count = 0;                    
        adc_channels[i].min = 0;
        adc_channels[i].max = 0;
        adc_channels[i].res = 0;
    }

    adc_ch = 0;
    
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    adc_start_measure();

    for(;;) {
        if(0 == ADCON0bits.GO_nDONE) {
             
#asm
            clrwdt
#endasm
            
            switch(ADCON0bits.CHS) {
                case channel_AN3:
                    i = 0;
                    break;

                case channel_AN7:
                    i = 1;
                    break;

                case channel_AN2:
                    i = 2;
                    break;

                case channel_AN6:
                    i = 3;
                    break;

                default:
                    continue;
            }
            
            pchn = &adc_channels[i];

            pchn->buf[0] = pchn->buf[1];
            pchn->buf[1] = pchn->buf[2];
            pchn->buf[2] = pchn->buf[3];
            pchn->buf[3] = pchn->buf[4];
            pchn->buf[4] = (ADRESH << 8) | ADRESL;

            result = (pchn->buf[0]*-3) + (pchn->buf[1]*12) + (pchn->buf[2]*17) + (pchn->buf[3]*12) - (pchn->buf[4]*3);
            smpl_val = (adc_result_t)(result / 35);
            
            if(0 == pchn->count) {
                pchn->min = smpl_val;
                pchn->max = smpl_val;
            } else {
                if(smpl_val > pchn->max)
                    pchn->max = smpl_val;
                
                if(smpl_val < pchn->min)
                    pchn->min = smpl_val;
            }

            if(++pchn->count >= SAMPLES_MAX) {
#ifdef USE_BLINKER
                blink();
#endif
                uint16_t range = pchn->max - pchn->min;
                tmp = (float)range / 2.0 * 1.41421356;  // 1.41421356 is a approximate value of the square root of 2
                tmp = tmp / 409.6 * 50.0 * 1000.0;
                
                pchn->count = 0;                 
                pchn->res = (uint16_t)tmp;
            }
            
            adc_ch = (adc_ch + 1) & 3;
            
            adc_start_measure();
        }
    }
}

