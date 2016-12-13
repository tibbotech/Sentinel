/** 
 * @file   eeprom.h  It contains declarations of methods to work with eeprom  
 */

#ifndef EEPROM_H
#define	EEPROM_H

#include "mtypes.h"

//! function reads one byte from the specified address eeprom
unsigned char ReadEEPROM(unsigned char address);

//! procedure writes one byte to the specified address eeprom
void WriteEEPROM(unsigned char address, unsigned char data);


#endif	/* EEPROM_H */