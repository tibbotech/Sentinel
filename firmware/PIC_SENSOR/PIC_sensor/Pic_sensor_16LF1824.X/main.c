/** 
 * @file   main.c  This program allows you to read data from I2C sensors and give the data on RS-485 upon request master. You should select the type of sensor using #define SENSOR
 * @author Mikhail
 *
 * Created on 18 may 2016  13:51
 */

/** 
@mainpage 
 *  This program allows you to read data from I2C sensors and give the data on RS-485 upon request master. You should select the type of sensor using #define SENSOR
 *  This project should have the following properties for use with bootloader AN1310,
 *  located at the end of the program memory :
 *  Project properties --> XC8 linker --> Option categories: Memory model --> ROM ranges: default,-E10-FFF
*/ 


//#define PROTOTYPE
#define MOTHERBOARD

//...............................................................SENSORS.............................................................................................//
//#define SENSOR 0x01
#define SENSOR 0x02
//#define SENSOR 0x03
//#define SENSOR 0x04
//#define SENSOR 0x05
//...............................................................SENSORS.............................................................................................//

//type_sensor = 0x01h  'A' ACCEL
//type_sensor = 0x02h  'H' HUMIDITY
//type_sensor = 0x03h  'L' LIGHT
//type_sensor = 0x04h  'T' TEMPERATURE
//type_sensor = 0x05h  'F' FLOOD

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

#ifdef PROTOTYPE
#include <pic16f1824.h>
#endif

#ifdef MOTHERBOARD
#include <pic16lf1824.h>
#endif

#include "hardw_i2c.h"
#include "uart.h"
#include "eeprom.h"
#include "buffer.h"

#define _XTAL_FREQ 16000000

//! CONFIG1
#pragma config FOSC = INTOSC                    ///< Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF                       ///< Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF                      ///< Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF                      ///< MCLR Pin Function Select (MCLR/VPP pin function is input only)
#pragma config CP = OFF                         ///< Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF                        ///< Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON                       ///< Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF                   ///< Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON                        ///< Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON                       ///< Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)


//! CONFIG2
#pragma config WRT = OFF                        ///< Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON                       ///< PLL Enable (4x PLL enabled)
#pragma config STVREN = ON                      ///< Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO                        ///< Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF                        ///< Low-Voltage Programming Disabled 


unsigned char rxdata = 0;                       ///< global temp variable for reading from RX buffer
unsigned char temp_rxdata = 0;                  ///< global temp variable for reading second param CMD from RX buffer 
unsigned char i2c_data_hight = 0;               ///< global temp variable for reading most significant data byte from I2C bus
unsigned char i2c_data_low = 0;                 ///< global temp variable for reading least significant data byte from I2C bus
unsigned char i2c_data_hight_2 = 0;             ///< global temp variable for reading most significant data byte from I2C bus
unsigned char i2c_data_low_2 = 0;               ///< global temp variable for reading least significant data byte from I2C bus
unsigned char i2c_data_hight_3 = 0;             ///< global temp variable for reading most significant data byte from I2C bus
unsigned char i2c_data_low_3 = 0;               ///< global temp variable for reading least significant data byte from I2C bus
volatile unsigned char type_sensor = 0;         ///< global temp variable for storing type of sensor
int ID_int = 0;                                 ///< global temp variable for storing ID for procedure int_send()
unsigned char ID_1 = 0;                         ///< global temp variable for storing first parth of ID
unsigned char ID_2 = 0;                         ///< global temp variable for storing second parth of ID
unsigned char ID_3 = 0;                         ///< global temp variable for storing third parth of ID
unsigned char ID_4 = 0;                         ///< global temp variable for storing fourth parth of ID
unsigned char ID_1_rx = 0;                      ///< global temp variable for storing first parth of received ID
unsigned char ID_2_rx = 0;                      ///< global temp variable for storing second parth of received ID
unsigned char ID_3_rx = 0;                      ///< global temp variable for storing third parth of received ID
unsigned char ID_4_rx = 0;                      ///< global temp variable for storing fourth parth of received ID
unsigned char SN_1_rx = 0;                      ///< global temp variable for storing first parth of received SN
unsigned char SN_2_rx = 0;                      ///< global temp variable for storing second parth of received SN
unsigned char SN_3_rx = 0;                      ///< global temp variable for storing third parth of received SN
unsigned char SN_4_rx = 0;                      ///< global temp variable for storing fourth parth of received SN
//int SN_rx = 0;                                ///< global temp variable for storing received SN
extern volatile boolean   CR_flag;              ///< global variable for storing state of received packet of data       
extern volatile boolean   STX_flag;             ///< global variable for storing state of received packet of data                     
unsigned char Temperature = 0;                  ///< global variable for storing integer part of temperature from temperature sensor
unsigned char frac_Temperature = 0;             ///< global variable for storing fraction part of temperature from temperature sensor
volatile boolean flag_init_sensor  = FALSE;     ///< global variable for storing state of sensor
volatile unsigned char flag_refresh_X  = 0;     ///< global variable for storing state of X-axis accelerometer sensor
volatile unsigned char flag_refresh_Y  = 0;     ///< global variable for storing state of Y-axis accelerometer sensor
volatile unsigned char flag_refresh_Z  = 0;     ///< global variable for storing state of Z-axis accelerometer sensor
volatile boolean flag_refresh_sensor  = FALSE;  ///< global variable for storing state of sensor
unsigned char resolv_data = 0;                  ///< global variable for storing resolution of temperature sensor           
int p_counter = 0;                              ///< global counter for str_buffer[]
unsigned char str_buffer[6] = {0};              ///< global mass for procedure int2str() and int_send()
unsigned int i2c_int = 0;                       ///< global temp variable for storing temp data from I2C bus and using this type in procedure int_send()
unsigned int i2c_int_prev = 0;                  ///< global temp variable for storing previous temp data from I2C bus
int X_axis = 0;                                 ///< global variable for storing value of X-axis accelerometer sensor
int Y_axis = 0;                                 ///< global variable for storing value of Y-axis accelerometer sensor
int Z_axis = 0;                                 ///< global variable for storing value of Z-axis accelerometer sensor
int X_axis_prev = 0;                            ///< global variable for storing previous value of X-axis accelerometer sensor
int Y_axis_prev = 0;                            ///< global variable for storing previous value of Y-axis accelerometer sensor
int Z_axis_prev = 0;                            ///< global variable for storing previous value of Z-axis accelerometer sensor
int HIH_temp = 0;                               ///< global variable for storing value of temperature humidity sensor
int HIH_hum = 0;                                ///< global variable for storing value of humidity humidity sensor
int LED_counter = 0;                            ///< global variable for LED blinking

extern cBuffer uartRxBuffer;                    ///< extern uart receive buffer (see uart.c)                        
extern cBuffer uartTxBuffer;                    ///< extern uart transmit buffer (see uart.c)                        


 /**
 procedure translate received ASCII data (only sensor ID) from uart RX buffer to one byte
 */
unsigned char str2chr()
{
   unsigned char temp = 0;
   unsigned char rtrID = 0; 
    
   uartReceiveByte(&rxdata);
   rtrID = rxdata - '0';
   uartReceiveByte(&rxdata);
   if ((rxdata == 0x2E) || (rxdata == 0x47) || (rxdata == 0x53) || (rxdata == 0x0D) || (rxdata == 0x20)) return rtrID; 
   rtrID = rtrID * 10;
   temp = rxdata - '0';
   uartReceiveByte(&rxdata);
   if ((rxdata == 0x2E) || (rxdata == 0x47) || (rxdata == 0x53) || (rxdata == 0x0D) || (rxdata == 0x20)) {rtrID = rtrID + temp; return rtrID;}
   rtrID = rtrID * 10;
   temp = temp * 10;
   rtrID = rtrID + temp;
   temp = rxdata - '0';
   rtrID = rtrID + temp;
   uartReceiveByte(&rxdata);
   if ((rxdata == 0x2E) || (rxdata == 0x47) || (rxdata == 0x53) || (rxdata == 0x0D) || (rxdata == 0x20)) return rtrID; 
   return 0;
   
}
 
/**
 procedure translate received ASCII SN from uart RX buffer to one byte
 */
unsigned char SN2chr()
{
    uartReceiveByte(&rxdata);
    return rxdata;
    
   //unsigned int temp_int = 0;
   //int rtrSN = 0;
    
   //uartReceiveByte(&rxdata);
   //rtrSN = rxdata - '0';
   //rtrSN = rtrSN * 1000;
   //uartReceiveByte(&rxdata);
   //temp_int = rxdata - '0';
   //temp_int = temp_int * 100;
   //rtrSN = rtrSN + temp_int;
   //uartReceiveByte(&rxdata);
   //temp_int = rxdata - '0';
   //temp_int = temp_int * 10;
   //rtrSN = rtrSN + temp_int;
   //uartReceiveByte(&rxdata);
   //temp_int = rxdata - '0';
   //rtrSN = rtrSN + temp_int;
   //uartReceiveByte(&rxdata);
   //return rtrSN;
}


/**
 procedure translates integer number to mass of ASCII data for transmit via uart module
 @param n - integer number
 */
void int2str(unsigned int n)
{     
   p_counter = 1;  
   while (n > 9)
   {
       str_buffer[p_counter++] = (n % 10) + '0';
       n = n / 10;
   }
   str_buffer[p_counter++] = n + '0'; 
 }
 
 
/**
procedure sends mass of ASCII data via uart module
@param data - integer number which translates to mass of ASCII
*/
void int_send (int data) 
{
   int2str(data);
   while (p_counter--)
   {     
        uart_bufferAddToEnd(&uartTxBuffer, str_buffer[p_counter]);
        if (p_counter==1) {p_counter--;}
   }
}
  
/**
procedure does delay proportionally x
@param x - integer number which proportionally delay time
*/
void delay_main(int x)
{
    for(int i=0;i<100*x;i++)
    {

    }
}

/**
procedure initialize CLOCK
*/
void CLOCK_setup(void)
{
	OSCCONbits.SCS = 0b11;                  ///< Use internal Clock defined by IRCF
    OSCCONbits.IRCF = 0b1111;               ///< Set clock to 16 MHz
}

/**
procedure blinking of LED 1
*/
void LED1_blink(void)
{
    LED_counter++;
    if (LED_counter < 255) 
    { 
     LATAbits.LATA2 = 1; 
    }    
    if (LED_counter > 255) 
    { 
     LATAbits.LATA2 = 0; 
    }  
    if (LED_counter > 511) 
    { 
     LED_counter = 0; 
    }
    
    if (PORTAbits.RA3 == 0)
    {
     LATCbits.LATC2 = 0;    
    } 
    else 
    {
     LATCbits.LATC2 = 1;   
    }    
    //LATAbits.LATA2 = 0;
    //__delay_ms(100);
    //LATAbits.LATA2 = 1;
    //__delay_ms(100);
}

/**
procedure blinking of LED 2
*/
void LED2_blink(void)
{
    #ifdef PROTOTYPE
        LATAbits.LATA5 = 1;
        //__delay_ms(1000);
        delay_main(500);
        LATAbits.LATA5 = 0;
        //__delay_ms(1000);
        delay_main(500);
    #endif 
}

/**
procedure initialize of LEDs
*/
void Init_LEDS(void)
{
    TRISAbits.TRISA2 = 0;                   ///< setting port RA2 as output is also used by ADC_init()
    LATAbits.LATA2 = 1;                     ///< RA2 setting hight level for RTS second 05 tibbit RS-485
    
    TRISAbits.TRISA3 = 1;                   ///< MCLR as input button   
    OPTION_REGbits.nWPUEN =0b0 ;            ///< Weak pull-ups are enabled by individual WPUx latch values
    WPUAbits.WPUA3 = 1;                     ///< pull-up MCLR
    
    TRISCbits.TRISC2 = 0;                   ///< RC2 as output RTS for RS-485
    LATCbits.LATC2 = 1;                     ///< RC2 setting low level for RTS
    
    
    #ifdef MOTHERBOARD     
        TRISCbits.TRISC3 = 0;               ///< RC3 as output RTS for RS-485 MOTHERBOARD
        LATCbits.LATC3 = 0;                 ///< RC3 setting low level for RTS
    #endif    
     
    #ifdef MOTHERBOARD
        #if (SENSOR==0x03)
        TRISAbits.TRISA5 = 0;              ///< setting port RA5 as output for DVI
        LATAbits.LATA5 = 1;                ///< RA5 setting hight level for DVI
        #endif
    #endif 
        
        
    #ifdef PROTOTYPE
        TRISAbits.TRISA5 = 0;              ///< setting port RA5 as output for RS-485 PROTOTYPE
        LATAbits.LATA5 = 0;                ///< RA5 setting low level for RTS
    #endif
}

/**
procedure translate raw axis value from sensor to range +/- 1.5g
@param axis - raw axis value
*/
int mg_calc (int axis)
{  
 int XYZ_axis_temp = 0;   
 XYZ_axis_temp = (axis * 29)/10;
 axis = XYZ_axis_temp;
 return axis;      
}

/**
procedure adding sign if it is needed
@param axis - raw axis value
*/
int sign_result (int axis)
{
 if (axis > 511) 
 {
    axis = 0x0400 - axis;
    uart_bufferAddToEnd(&uartTxBuffer, 0x2D);
 }
 return axis; 
}

/**
procedure sends specific preambula of the transmitted packet
*/
void uart_send_preambula (void)
{

    uart_bufferAddToEnd(&uartTxBuffer, 0x02);                   ///< STX
    ID_int = ID_1;
    int_send(ID_int);
    uart_bufferAddToEnd(&uartTxBuffer, 0x2E);                   ///< '.' it means dot
    ID_int = ID_2;
    int_send(ID_int);
    uart_bufferAddToEnd(&uartTxBuffer, 0x2E);
    ID_int = ID_3;
    int_send(ID_int);
    uart_bufferAddToEnd(&uartTxBuffer, 0x2E);
    ID_int = ID_4;
    int_send(ID_int);

    
    uart_bufferAddToEnd(&uartTxBuffer, 0x20);                   ///< ' ' it means spacer
    uart_bufferAddToEnd(&uartTxBuffer, SN_1_rx);
    uart_bufferAddToEnd(&uartTxBuffer, SN_2_rx);
   // uart_bufferAddToEnd(&uartTxBuffer, SN_3_rx);
   // uart_bufferAddToEnd(&uartTxBuffer, SN_4_rx);
    //int_send(SN_rx);
    uart_bufferAddToEnd(&uartTxBuffer, 0x20);                   ///< ' ' it means spacer
}



/**
procedure initialize of ADC
*/
void ADC_init (void)
{   
    
    #ifdef PROTOTYPE
    ANSELCbits.ANSC2 = 1;                                       ///< setting port RC2 as analog input
    WPUCbits.WPUC2 = 0;                                         ///< disable pull-up
    TRISCbits.TRISC2 = 1;                                       ///< setting port RC2 as input
    #endif
    
    #ifdef MOTHERBOARD 
    ANSELAbits.ANSA4 = 1;                                       ///< setting port RA4 as analog input
    WPUAbits.WPUA4 = 0;                                         ///< disable pull-up
    TRISAbits.TRISA4 = 1;                                       ///< setting port RA4 as input
    #endif
    
    
    #ifdef PROTOTYPE
    ADCON0bits.CHS = 0b00010;                                   ///< AN6
    #endif  
    
    #ifdef MOTHERBOARD
    ADCON0bits.CHS = 0b00011;                                   ///< AN3
    #endif 
    
     
    ADCON1bits.ADNREF = 0;                                      ///< VREF- is connected to VSS
    ADCON1bits.ADPREF = 0b00;                                   ///< VREF+ is connected to VDD
    ADCON1bits.ADCS = 0b101;                                    ///< Fosc/64
    ADCON1bits.ADFM = 1;                                        ///< Right justified
    ADCON0bits.ADON = 1;                                        ///< ADC is enabled    
}

/**
procedure reading value from ADC
@return temp_ADC - value from ADC
*/
int ADC_calc ()
{
    int temp_ADC = 0;
    
    ADGO=1;
    while(ADGO==1)
		{
		}

        temp_ADC = ADRESH;
        temp_ADC = temp_ADC & 0x0003;
        temp_ADC = temp_ADC * 256;
    
		temp_ADC |= ADRESL;	
	
        return temp_ADC;        
}


void main() 
{
    
    CLOCK_setup();
    Init_LEDS();
    ADC_init();
    uart_init();
    i2c_init_hardw();
   
        
    while (1) 
   {
        LED1_blink();                                                           ///<indicate state of sensor
        
        ID_1 = ReadEEPROM(0x00);                             
        ID_2 = ReadEEPROM(0x01);
        ID_3 = ReadEEPROM(0x02);
        ID_4 = ReadEEPROM(0x03);
         
        type_sensor = SENSOR;
        //type_sensor = ID_1;
        //type_sensor = (ID >> 24) & 0xFF;
        //type_sensor = 0x01h  'A' ACCEL
        //type_sensor = 0x02h  'H' HUMIDITY
        //type_sensor = 0x03h  'L' LIGHT
        //type_sensor = 0x04h  'T' TEMPERATURE
        //type_sensor = 0x05h  'F' FLOOD
        
        //type_sensor = 0x41h  'A' ACCEL
        //type_sensor = 0x48h  'H' HUMIDITY
        //type_sensor = 0x4Ch  'L' LIGHT
        //type_sensor = 0x54h  'T' TEMPERATURE
        //type_sensor = 0x46h  'F' FLOOD
        
               
       #if SENSOR == 0x01       
        if  (type_sensor == 0x01)                           ///< ACCEL SENSOR   
       {
          if (!flag_init_sensor)         
           {
                /*i2c_idle();
                i2c_start();
                i2c_send_address(0xA6);                     ///< write command
                i2c_send_data(0x2D);                        ///< Power saving features control reg
                i2c_idle();
                i2c_send_data(0x00);                        ///< reset REG
                i2c_idle();
                i2c_stop();*/
                i2c_write_reg(0xA6, 0x2D, 0x00);    
    
         
                /*i2c_idle();
                i2c_start();
                i2c_send_address(0xA6);                     ///< write command
                i2c_send_data(0x2D);                        ///< Power saving features control reg
                i2c_idle();
                // i2c_send_data(0x18);                     ///< (0x08)set measure bit!!!! (0x10) set auto_sleep
                i2c_send_data(0x08);                        ///< (0x08)set measure bit!!!!
                i2c_idle();
                i2c_stop();*/
                i2c_write_reg(0xA6, 0x2D, 0x08);  
     
                /*i2c_idle();
                i2c_start();
                i2c_send_address(0xA6);                     ///< write command
                i2c_send_data(0x38);                        ///< FIFO control reg
                i2c_idle();
                // i2c_send_data(0x80);                     ///< stream mode
                i2c_send_data(0x00);                        ///< bypass mode
                i2c_idle();
                i2c_stop();*/
                i2c_write_reg(0xA6, 0x38, 0x00); 
     
                /*i2c_idle();
                i2c_start();
                i2c_send_address(0xA6);                     ///< write command
                i2c_send_data(0x31);                        ///< Data format control reg
                i2c_idle();
                i2c_send_data(0x08);                        ///< full resolve - it means(MSB A5,A4,A3,A2,A1,A0) *2.9 // A0 + A1 = 00 - it means +/-1.5g
                i2c_idle();
                i2c_stop();*/
                i2c_write_reg(0xA6, 0x31, 0x08);
    
                flag_init_sensor  = TRUE;     
            }
     
            i2c_idle();
            i2c_start();
            i2c_send_address(0xA6);                         ///< write command
            i2c_send_data(0x32);                            ///< LSB reg X-axis
            i2c_idle();
            i2c_start();
            i2c_send_address(0xA7);                         ///< read  command 
            i2c_data_low = i2c_read_data();                 ///< read X-axis
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_hight = i2c_read_data();               ///< read X-axis
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_low_2 = i2c_read_data();               ///< read Y-axis
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_hight_2 = i2c_read_data();             ///< read Y-axis
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_low_3 = i2c_read_data();               ///< read Z-axis
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_hight_3 = i2c_read_data();             ///< read Z-axis
            i2c_idle();
            i2c_nak();
            i2c_idle();
            i2c_stop();  
     
     
            X_axis = i2c_data_hight * 256 + i2c_data_low;
            Y_axis = i2c_data_hight_2 * 256 + i2c_data_low_2;
            Z_axis = i2c_data_hight_3 * 256 + i2c_data_low_3;
     
            X_axis = X_axis & 0x03FF;
            Y_axis = Y_axis & 0x03FF;
            Z_axis = Z_axis & 0x03FF;
     
     
            //....................................................................X_axis......................................................................................//    
            if(flag_refresh_X==0)
            {                       
                    if ((X_axis_prev < 512) && (X_axis < 512))                                                              ///< X_axis_prev and X_axis = positiv number
                    {
                        if (X_axis > X_axis_prev) {X_axis_prev = X_axis;}                                                   ///< positiv number
                    }       
        
                    if ((X_axis_prev > 511) && (X_axis > 511))                                                              ///< X_axis_prev and X_axis = negativ number
                    {
                        if (X_axis_prev > X_axis)   {X_axis_prev = X_axis;}                                                 ///< negativ number
                    }
       
                    if ((X_axis_prev < 512) && (X_axis > 511))                                                              ///< X_axis_prev = positiv number //X_axis = negativ number
                    {
                        if ((1024 - X_axis) > X_axis_prev) {X_axis_prev = X_axis;}   
                    }
        
                    if ((X_axis_prev > 511) && (X_axis < 512))                                                              ///< X_axis_prev = negativ number //X_axis = positiv number
                    {
                        if ( X_axis > (1024 - X_axis_prev)) {X_axis_prev = X_axis;}   
                    }      
            }
            else
            {
            X_axis_prev = X_axis;
            flag_refresh_X = 0;                                                                                             ///< 0 = FALSE
            }  
             //................................................................................................................................................................//     
     
  
     
             //......................................................................Y_axis....................................................................................//     
            if(flag_refresh_Y==0)
            {
                if ((Y_axis_prev < 512) && (Y_axis < 512))                                      ///< Y_axis_prev and Y_axis = positiv number
                {
                    if (Y_axis > Y_axis_prev) {Y_axis_prev = Y_axis;}                           ///< positiv number
                }       
        
                if ((Y_axis_prev > 511) && (Y_axis > 511))                                      ///< Y_axis_prev and Y_axis = negativ number
                {
                    if (Y_axis_prev > Y_axis)   {Y_axis_prev = Y_axis;}                         ///< negativ number
                }
       
                if ((Y_axis_prev < 512) && (Y_axis > 511))                                      ///< Y_axis_prev = positiv number //Y_axis = negativ number
                {
                    if ((1024 - Y_axis) > Y_axis_prev) {Y_axis_prev = Y_axis;}   
                }
        
                if ((Y_axis_prev > 511) && (Y_axis < 512))                                      ///< Y_axis_prev = negativ number //Y_axis = positiv number
                {
                    if ( Y_axis > (1024 - Y_axis_prev)) {Y_axis_prev = Y_axis;}   
                } 
            }
            else
            {
                Y_axis_prev = Y_axis;
                flag_refresh_Y =  0;                                                            ///< 0 = FALSE
            } 
     
    
            //..........................................................................................................................................................//     
     
     
            //.......................................................................Z_axis...................................................................................//     
            if(flag_refresh_Z==0)
                
           {
                
                if ((Z_axis_prev < 512) && (Z_axis < 512))                                      ///< Z_axis_prev and Z_axis = positiv number
                {
                    if (Z_axis > Z_axis_prev) {Z_axis_prev = Z_axis;}                           ///< positiv number
                }       
        
                if ((Z_axis_prev > 511) && (Z_axis > 511))                                      ///< Z_axis_prev and Z_axis = negativ number
                {
                    if (Z_axis_prev > Z_axis)   {Z_axis_prev = Z_axis;}                         ///< negativ number
                }
       
                if ((Z_axis_prev < 512) && (Z_axis > 511))                                      ///< Z_axis_prev = positiv number //Z_axis = negativ number
                {
                    if ((1024 - Z_axis) > Z_axis_prev) {Z_axis_prev = Z_axis;}   
                }
        
                if ((Z_axis_prev > 511) && (Z_axis < 512))                                      ///< Z_axis_prev = negativ number //Z_axis = positiv number
                {
                    if ( Z_axis > (1024 - Z_axis_prev)) {Z_axis_prev = Z_axis;}   
                }  
            }
            else
            {
             Z_axis_prev = Z_axis;
             flag_refresh_Z =  0;                                                                ///< 0 = FALSE
            } 
            //..........................................................................................................................................................//              
     
            X_axis_prev = X_axis_prev & 0x03FF;                                                 ///< 10-bit only resolution
            Y_axis_prev = Y_axis_prev & 0x03FF;                                                 ///< 10-bit only resolution
            Z_axis_prev = Z_axis_prev & 0x03FF;                                                 ///< 10-bit only resolution
         
       
        }
          
        #endif  
          
          
        #if SENSOR == 0x02
          
        if (type_sensor == 0x02)                            ///< HUMIDITY SENSOR   
        {
            i2c_idle();
            i2c_start();
            i2c_send_address(0x4E);
            i2c_idle();
            i2c_stop();
     
            //__delay_ms(5);
     
            i2c_idle();
            i2c_start();
            i2c_send_address(0x4F);                         ///< read  command 
            i2c_idle();
            i2c_data_hight = i2c_read_data();               ///< read HIH_hum_H
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_low = i2c_read_data();                 ///< read HIH_hum_L
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_hight_2 = i2c_read_data();             ///< read HIH_temp_H
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_low_2 = i2c_read_data();               ///< read HIH_temp_L
            i2c_idle();
            i2c_nak();
            i2c_idle();
            i2c_stop();
     
        }       
        
        #endif  
          
          
        #if SENSOR == 0x03          
        
        if (type_sensor == 0x03)                        ///< LIGHT SENSOR 
        {     
            if (!flag_init_sensor)   
            { 
                /* i2c_idle();
                i2c_start();
                i2c_send_address(0x46 & 0xFE);          ///< & 0xFE it means write command
                i2c_send_data(0x00);                    ///< power_down cmd
                i2c_idle();
                i2c_stop(); */
                i2c_write_data(0x46, 0x00);   
      
                /*i2c_idle();
                i2c_start();
                i2c_send_address(0x46 & 0xFE);          ///< & 0xFE it means write command
                i2c_send_data(0x01);                    ///< power_on cmd
                i2c_idle();
                i2c_stop(); */
                i2c_write_data(0x46, 0x01);
      
                /*i2c_idle();
                i2c_start();
                i2c_send_address(0x46 & 0xFE);          ///< & 0xFE it means write command
                i2c_send_data(0x12);                    ///< BH_CMD_HRESOL_0 cmd
                i2c_idle();
                i2c_stop();*/
                i2c_write_data(0x46, 0x12);
      
                flag_init_sensor  = TRUE; 
            }
         
            i2c_idle();
            i2c_start();
            i2c_send_address(0x47);               
            i2c_data_hight = i2c_read_data();
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_low = i2c_read_data();
            i2c_idle();
            i2c_nak();
            i2c_idle();
            i2c_stop();
              
        }
        
        #endif        
        
        
        #if SENSOR == 0x04
          
        if (type_sensor == 0x04)                                                ///< TEMPERATURE SENSOR
        {
            i2c_idle();
            i2c_start();
            i2c_send_address(0x30);                                             ///< & 0xFE it means write command //A0 = 0; A1 = 0; A2 = 0;
            i2c_send_data(0x08);
            i2c_idle();
            i2c_start();
            i2c_send_address(0x31);  
            resolv_data = i2c_read_data();
            i2c_idle();
            i2c_nak();
            i2c_idle();
            i2c_stop();                                                         ///< Here read resolution temperature (by default resolv_data = 0x03 - i. e. + 0.0625C - higest resolv!!!))
         
         
            i2c_idle();
            i2c_start();
            i2c_send_address(0x30);                                             ///< & 0xFE it means write command //A0 = 0; A1 = 0; A2 = 0;
            i2c_send_data(0x05);
            i2c_idle();
            i2c_start();
            i2c_send_address(0x31);                                             ///< | 0x01 it means read command //A0 = 0; A1 = 0; A2 = 0;
            i2c_data_hight = i2c_read_data();
            i2c_idle();
            i2c_ack();
            i2c_idle();
            i2c_data_low = i2c_read_data();
            i2c_idle();
            i2c_nak();
            i2c_idle();
            i2c_stop();
              
            //Convert the temperature data
            //First Check flag bits
            if ((i2c_data_hight & 0x80) == 0x80) {}       //TA ³ TCRIT
            if ((i2c_data_hight & 0x40) == 0x40) {}       //TA > TUPPER
            if ((i2c_data_hight & 0x20) == 0x20) {}       //TA < TLOWER
            i2c_data_hight = i2c_data_hight & 0x1F;       //Clear flag bits
            
            if ((i2c_data_hight & 0x10) == 0x10)          //TA < 0°C  
            {
            i2c_data_hight = i2c_data_hight & 0x0F;       //Clear SIGN
            Temperature = 256 - (i2c_data_hight * 16 + i2c_data_low / 16);
            i2c_data_hight| = 0x10;
            }
            else Temperature = (i2c_data_hight * 16 + i2c_data_low / 16);
            frac_Temperature = i2c_data_low % 16;            
        }       
        
        #endif
          
                  
        #if SENSOR == 0x05  
          
        if (type_sensor == 0x05)                  /// FLOOD SENSOR
        {         
            //.....................................................................ADC...............................................................................// 
            i2c_int =  ADC_calc();
 
            if(!flag_refresh_sensor)
            {
                if (i2c_int > i2c_int_prev) {i2c_int_prev = i2c_int;} 
            }
            else
            {
                i2c_int_prev = i2c_int;
                flag_refresh_sensor = FALSE;                  
            }        
           
            //.....................................................................ADC...............................................................................//   
        }          
        #endif         
          
               
    
       if ((uartRxBuffer.datalength != uartRxBuffer.dataindex) && (CR_flag))    ///< if not empty
       {
                           
            while (uartRxBuffer.datalength != uartRxBuffer.dataindex) 
            {        
                uartReceiveByte(&rxdata);
                if (rxdata == 0x02)                                             ///< receive [STX]
                {
                    ID_1_rx = str2chr();
                    ID_2_rx = str2chr();
                    ID_3_rx = str2chr();
                    ID_4_rx = str2chr();
                    
                    SN_1_rx = SN2chr();
                    SN_2_rx = SN2chr();
                   // SN_3_rx = SN2chr();
                   // SN_4_rx = SN2chr();
                    uartReceiveByte(&rxdata);                                   ///< 0x20 (spacer))
                    
                    if ((ID_1_rx==ID_1) && (ID_2_rx==ID_2) && (ID_3_rx==ID_3) && (ID_4_rx==ID_4))
                    {
                        uartReceiveByte(&rxdata);
                        switch (rxdata)                                                             ///< CMD
                        {                                                     
                            case (0x47):                                                            ///< 'G' command GET 
                                                     
                            uartReceiveByte(&temp_rxdata);                                          ///< receive second param CMD                
                      
                            #if SENSOR == 0x04
                            if ((temp_rxdata == 0x30) && (type_sensor == 0x04 ))                    ///< GET '0x30' = GET 'T' (GET '0x54') if type_sensor == 'TEMPERATURE' = 0x04h
                        
                            { 
                                // Answer to GET TEMPERATURE 
                                uart_send_preambula();                              
                                                                                                 
                                i2c_int = Temperature ;
                                if ((i2c_data_hight & 0x10) == 0x10)                               
                                {
                                 uart_bufferAddToEnd(&uartTxBuffer, 0x2D);                          ///< 0x2D = minus                            
                                }
                            
                                int_send(i2c_int);
                                uart_bufferAddToEnd(&uartTxBuffer, 0x2E);                           ///< 0x2E = dot
                                
                                if (frac_Temperature ==0)
                                {
                                  uart_bufferAddToEnd(&uartTxBuffer, 0x30);
                                  uart_bufferAddToEnd(&uartTxBuffer, 0x30);
                                  uart_bufferAddToEnd(&uartTxBuffer, 0x30);
                                  uart_bufferAddToEnd(&uartTxBuffer, 0x30);
                                }
                                    
                                if (frac_Temperature ==1) 
                                {
                                 uart_bufferAddToEnd(&uartTxBuffer, 0x30);
                                 i2c_int = 625;
                                 int_send(i2c_int);
                                }
                                else
                                {
                                 if (frac_Temperature !=0) 
                                 {    
                                  i2c_int = frac_Temperature * 625;
                                  int_send(i2c_int);
                                 }
                                }
                               
                                                           
                                uart_bufferAddToEnd(&uartTxBuffer, 0x52);                            ///< 0x52 = 'R' It means result ASCII 
                                uart_bufferAddToEnd(&uartTxBuffer, 0x30);                            ///< '0x30' parametr
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                            ///< CR  without checksumm
                         
                                break;
                            }
                            #endif
                      
                      
                            #if SENSOR == 0x02
                            if ((temp_rxdata == 0x30) && (type_sensor == 0x02 ))                     ///< GET '0x30' = GET 'H'(GET '0x48') if type_sensor == 'HUMIDITY' = 0x02h
                    
                            { 
                                // Answer to GET HUMIDITY 
                                uart_send_preambula();                               
                          
                                //HIH_hum;
                                i2c_data_hight = i2c_data_hight & 0x3F ;      
                                HIH_hum = i2c_data_hight * 256 + i2c_data_low ; 
                         
                                //HIH_hum = HIH_hum /16382;                                    ///< integer part of HUMIDITY
                                i2c_int = HIH_hum /164;                                        ///< i2c_int to output
                                int_send(i2c_int);
                                uart_bufferAddToEnd(&uartTxBuffer, 0x2E);                      ///< .point
                         
                                //i2c_data_low = HIH_hum % 16382;                              ///< frac part of HUMIDITY
                                i2c_int = HIH_hum % 164;                                       ///< frac part of HUMIDITY     
                                i2c_int = (i2c_int * 100) / 164;                               ///< integer arithmetic
                                int_send(i2c_int);                                             ///< frac part of HUMIDITY to output 
                           
                                uart_bufferAddToEnd(&uartTxBuffer, 0x52);                      ///< 0x52 = 'R' It means result ASCII 
                                uart_bufferAddToEnd(&uartTxBuffer, 0x30);                      ///< '0x30' parametr
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                      ///< CR  without checksumm
                                break;
                            }
                   
                                                                                        
                            if ((temp_rxdata == 0x31) && (type_sensor == 0x02 ))                ///< GET '0x31' = GET 'T' (GET '0x54') if type_sensor == 'HUMIDITY' = 0x02h                    
                            { 
                                // Answer to GET TEMPERATURE
                                uart_send_preambula();
                    
                                //HIH_temp;
                                HIH_temp = i2c_data_hight_2 / 4 ;                               ///< HIH_temp hight byte
                                i2c_data_hight_2 = i2c_data_hight_2 /16;                        ///< two MSB bits of HIH_temp low byte
                                i2c_data_hight_2 = i2c_data_hight_2 & 0x03;                     ///< masking (do not need, but it is here))
                                i2c_data_hight_2 = i2c_data_hight_2 * 64;                       ///< because there are two MSB bits of HIH_temp low byte
                        
                                i2c_int = (i2c_data_low_2 & 0xC0)/4 + i2c_data_hight_2;         ///< calculate MSB part(4 bits) of HIH_temp low byte
                                i2c_data_low_2 = i2c_data_low_2 & 0x3C;                         ///< calculate LSB part(4 bits) of HIH_temp low byte
                                i2c_data_low_2 = i2c_data_low_2 / 4;                            ///< calculate LSB part(4 bits) of HIH_temp low byte
                                i2c_int = i2c_int + i2c_data_low_2;                             ///< calculate HIH_temp low byte
                        
                        
                                HIH_temp = HIH_temp * 256 + i2c_int ;                           ///< calculate HIH_temp            
                         
                                //HIH_temp = HIH_temp /16382;                                   ///< integer part of TEMPERATURE
                                //i2c_int = ((HIH_temp /164) * 165) - 40;                       ///< i2c_int to output
                                i2c_int = HIH_temp /164;
                                i2c_int = ((i2c_int * 165)/100) - 40;
                                    
                                if (i2c_int > 32767) 
                                {
                                uart_bufferAddToEnd(&uartTxBuffer, 0x2D);                       ///< 0x2D = minus 
                                i2c_int = 65535 - i2c_int + 1;                                  ///<  +1 = 65536
                                }
                                int_send(i2c_int);
                                uart_bufferAddToEnd(&uartTxBuffer, 0x2E);                       ///< 0x2E = dot 
                        
                         
                                //i2c_data_low = HIH_temp % 16382;                              ///< frac part of TEMPERATURE
                                i2c_int = HIH_temp % 164;                                       ///< frac part of TEMPERATURE     
                                i2c_int = (i2c_int * 100) / 164;                                ///< integer arithmetic
                                int_send(i2c_int);                                              ///< frac part of TEMPERATURE to output                               
                    
                         
                                uart_bufferAddToEnd(&uartTxBuffer, 0x52);                       ///< 0x52 = 'R' It means result ASCII 
                                uart_bufferAddToEnd(&uartTxBuffer, 0x31);                       ///< '0x31' parametr
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                       ///< CR  without checksumm
                                break;
                            }
                            #endif
                      
                      
                            #if SENSOR == 0x01
                            if ((temp_rxdata == 0x30) && (type_sensor == 0x01 ))                ///< GET '0x30' = GET 'X'(GET '0x58') if type_sensor == 'ACCEL' = 0x01h                           
                            { 
                                //! Answer to GET ACCELERATION 
                                uart_send_preambula();
                                X_axis_prev = sign_result(X_axis_prev);
                                int_send(mg_calc(X_axis_prev));
                                   
                                flag_refresh_X = 0x01;
        
                                uart_bufferAddToEnd(&uartTxBuffer, 0x52);                       ///< 0x52 = 'R' It means result ASCII 
                                uart_bufferAddToEnd(&uartTxBuffer, 0x30);                       ///< '0x30' parametr
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                       ///< CR  without checksumm   
                        
                                break;
                            }
                     
                      
                      
                            if ((temp_rxdata == 0x31) && (type_sensor == 0x01 ))                ///< GET '0x31' = GET 'Y'(GET '0x59') if type_sensor == 'ACCEL' = 0x01h
                            { 
                                // Answer to GET ACCELERATION 
                                uart_send_preambula();
                               
                                Y_axis_prev = sign_result(Y_axis_prev);
                                int_send(mg_calc(Y_axis_prev));
                                  
                                flag_refresh_Y = 0x01;
        
                                uart_bufferAddToEnd(&uartTxBuffer, 0x52);                      ///< 0x52 = 'R' It means result ASCII 
                                uart_bufferAddToEnd(&uartTxBuffer, 0x31);                      ///< '0x31' parametr
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                      ///< CR  without checksumm                          
                                break;
                            }
                      
                                            
                            if ((temp_rxdata == 0x32) && (type_sensor == 0x01 ))               ///< GET '0x32' = GET 'Z' (GET '0x5A') if type_sensor == 'ACCEL' = 0x01h                           
                            { 
                                // Answer to GET ACCELERATION 
                                uart_send_preambula();
                              
                                Z_axis_prev = sign_result(Z_axis_prev);
                                int_send(mg_calc(Z_axis_prev));
                                                        
                                flag_refresh_Z = 0x01;
        
                                uart_bufferAddToEnd(&uartTxBuffer, 0x52);                       ///< 0x52 = 'R' It means result ASCII 
                                uart_bufferAddToEnd(&uartTxBuffer, 0x32);                       ///< '0x32' parametr
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                       ///< CR  without checksumm   
                        
                                break;
                            }
                            #endif
                      
                      
                            #if SENSOR == 0x03 
                            if ((temp_rxdata == 0x30) && (type_sensor == 0x03 ))                ///< GET '0x30' = GET 'L' (GET '0x4C') if type_sensor == 'LIGHT' = 0x03h
                            { 
                                // Answer to GET LIGHT
                                uart_send_preambula();
                                 
                                i2c_int = i2c_data_hight * 256 + i2c_data_low ;                              
                                i2c_int = (i2c_int * 10) /12;                                   ///< translate in luxury
                                int_send(i2c_int);
                        
                                uart_bufferAddToEnd(&uartTxBuffer, 0x52);                       ///< 0x52 = 'R' It means result ASCII 
                                uart_bufferAddToEnd(&uartTxBuffer, 0x30);                       ///< '0x30' parametr
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                       ///< CR  without checksumm
                                break;
                            }
                            #endif
                                           
                            #if SENSOR == 0x05
                            if ((temp_rxdata == 0x30) && (type_sensor == 0x05 ))                ///< GET '0x30' = GET 'F' (GET '0x46') if type_sensor == 'FLOOD' = 0x05h
                            { 
                                // Answer to GET FLOOD 
                                uart_send_preambula();
                                                     
                                //int_send(i2c_int_prev);
                                
                                if (i2c_int_prev > 350)
                                {                           
                                 uart_bufferAddToEnd(&uartTxBuffer, 0x31);                       ///< 0x31 = TRUE
                                }
                                else 
                                {
                                 uart_bufferAddToEnd(&uartTxBuffer, 0x30);                       ///< 0x30 = FALSE
                                }    
                                                                                   
                                flag_refresh_sensor = TRUE;
                        
                                uart_bufferAddToEnd(&uartTxBuffer, 0x52);                        ///< 0x52 = 'R' It means result ASCII 
                                uart_bufferAddToEnd(&uartTxBuffer, 0x30);                        ///< '0x30' parametr
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                        ///< CR  without checksumm
                                break;
                            }
                            #endif
                            
                            break;
                                                            
                            case (0x53):                                                      ///< 'S' command SET 
                         
                            uartReceiveByte(&temp_rxdata);                                    ///< receive second param CMD
                      
                            if (temp_rxdata == 0x49 )                                         ///< SET 'I' 
                            { 
                                //! SET 'I'                              
                                flag_init_sensor = FALSE;
                        
                                ID_1_rx = str2chr();
                                WriteEEPROM(0x00, ID_1_rx);
                                
                                ID_2_rx = str2chr();
                                WriteEEPROM(0x01, ID_2_rx);
            
                                ID_3_rx = str2chr();
                                WriteEEPROM(0x02, ID_3_rx);
                  
                                ID_4_rx = str2chr();
                                WriteEEPROM(0x03, ID_4_rx);
                                                                                             
                                //! Answer OK 
                                uart_send_preambula();
                                uart_bufferAddToEnd(&uartTxBuffer, 0x4F);                       ///< 'O' it means Answer
                                uart_bufferAddToEnd(&uartTxBuffer, 0x4B);                       ///< 'K' it means Answer
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                       ///< CR  without checksumm
                                                  
                                break;
                            }
                      
                            break;
                      
                        } /// end of switch
                      
                    }  /// end of  if (ID_rx == ID)  
                   
          
          
          
                    if ((ID_1_rx==0) && (ID_2_rx==0) && (ID_3_rx==0) && (ID_4_rx==0))           ///< broadcast                  
                    {     
                        uartReceiveByte(&rxdata);
                        switch (rxdata)                                                         ///< CMD
                        {                   
                            case (0x47):                                                        ///< 'G' command GET 
                          
                            uartReceiveByte(&temp_rxdata);                                      ///< receive second param CMD  
                          
                            #ifdef MOTHERBOARD
                            if ((temp_rxdata == 0x49) && (PORTAbits.RA3 == 0))                  ///< GET 'I' it means GET ID regardless type_sensor  //to do press button MD
                            #endif
                                
                            #ifdef PROTOTYPE    
                            if (temp_rxdata == 0x49)
                            #endif    
                                
                            { 
                                /// Answer to GET ID
                                uart_send_preambula();
                               
                                #if SENSOR == 0x01
                                uart_bufferAddToEnd(&uartTxBuffer, 0x33);                       ///< number of parameters
                                #endif
                                #if SENSOR == 0x02
                                uart_bufferAddToEnd(&uartTxBuffer, 0x32);                       ///< number of parameters
                                #endif
                                #if SENSOR == 0x03
                                uart_bufferAddToEnd(&uartTxBuffer, 0x31);                       ///< number of parameters
                                #endif
                                #if SENSOR == 0x04
                                uart_bufferAddToEnd(&uartTxBuffer, 0x31);                       ///< number of parameters
                                #endif
                                #if SENSOR == 0x05
                                uart_bufferAddToEnd(&uartTxBuffer, 0x31);                       ///< number of parameters
                                #endif
                                
                                uart_bufferAddToEnd(&uartTxBuffer, 0x0D);                       ///< CR  without checksumm
                                break;
                            } 
                            break; 
                        } ///end of switch(rxdata) //CMD
                    } ///end of  if (ID_rx == 0)   
          
                } ///end of if (rxdata == 0x02)
         
            } ///end of while (uartRxBuffer.datalength != uartRxBuffer.dataindex)
         
            CR_flag = FALSE;
            
        }   ///if ((uartRxBuffer.datalength != uartRxBuffer.dataindex) && (CR_flag))            ///< if not empty        
        
          
        if (uart_canSend())
        {       
            if (uartTxBuffer.datalength != uartTxBuffer.dataindex) 
            {   
                #ifdef PROTOTYPE   
                LATAbits.LATA2 = 0;                    ///< RA2 setting low level for RTS enable RX  
                LATAbits.LATA5 = 1;                    ///< RA5 setting hight level for RTS enable TX  
                #endif  

                #ifdef MOTHERBOARD  
                LATCbits.LATC3 = 1;                    ///< RC3 setting hight level for RTS
                #endif    
        
                
                uartSendTxBuffer(&uartTxBuffer); 
                while(!TXIF);                          ///< wait for previous transmission to finish
                __delay_ms(5);                         ///< need delay for transfer last byte 0x0D
     
                #ifdef MOTHERBOARD 
                LATCbits.LATC3 = 0;                    ///< RC3 setting low level for RTS
                #endif 

                #ifdef PROTOTYPE 
                LATAbits.LATA5 = 0;                    ///< RA5 setting low level for RTS
                LATAbits.LATA2 = 1;                    ///< RA2 setting hight level for RTS  
                #endif 

            }
     
        }          
    
    } // end of while (1)
    
} // end of main()

