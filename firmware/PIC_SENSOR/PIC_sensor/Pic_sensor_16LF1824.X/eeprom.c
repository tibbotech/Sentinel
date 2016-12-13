/** 
 * @file  eeprom.c It contains implementation of methods to work with eeprom 
 */

//#include <pic16f1824.h>
#include "eeprom.h"
#include <xc.h>


/**
 function reads one byte from the specified address eeprom
 @param address - an address of the eeprom memory
 */
unsigned char ReadEEPROM(unsigned char address)
{
    while(WR) continue;          
    EEADR=address;              
    EECON1 |= 0x01;             
    return EEDATA;  
}

/**
 procedure writes one byte to the specified address eeprom
 @param address - an address of the eeprom memory
 @param data - a data byte which writes to the specified address eeprom
 */
void WriteEEPROM(unsigned char address, unsigned char data)
{
    while(WR) continue;     
    EEADR=address;         
    EEDATA=data;           
    EECON1 |= 0x04;         
    CARRY=0;                
    if(GIE) CARRY=1;       
    GIE=0;               
    EECON2=0x55;          
    EECON2=0xAA;
    EECON1 |= 0x02;          
    EECON1 &= ~0x04;      
    if(CARRY) GIE=1;            
}




 


