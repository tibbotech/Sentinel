/** 
 * @file   hardw_i2c.h  It contains declarations of methods to work with hardware I2C module
 */

#ifndef HARDW_I2C_H
#define HARDW_I2C_H

#include<xc.h>
#include "mtypes.h"

//#define SDA_PIN PORTBbits.RB1
#define SDA_PIN  LATCbits.LATC1

//#define SCL_PIN PORTBbits.RB0	
#define SCL_PIN  LATCbits.LATC0

//#define SDA_DIR TRISBbits.RB1
#define SDA_DIR TRISCbits.TRISC1

//#define SCL_DIR TRISBbits.RB0
#define SCL_DIR TRISCbits.TRISC0


//! initialize a hardware I2C module to start at a given settings
void i2c_init_hardw ();

//! procedure writes one byte (address byte) to the I2C
void i2c_send_address(unsigned char address);

//! procedure writes one data byte to the I2C
void i2c_send_data(unsigned char data);

//! function reads one data byte from the I2C
unsigned char i2c_read_data(void);

//! procedure performs start of hardware I2C module
void i2c_start(void);

//! procedure performs stop of hardware I2C module
void i2c_stop(void);

//! procedure performs acknowledgment of receiving data from I2C bus
void i2c_ack(void);

//! procedure performs no acknowledgment of receiving data from I2C bus
void i2c_nak(void);

//! procedure performs waiting for the I2C bus to go into Idle mode 
void i2c_idle(void);

//! procedure performs repeating start condition
void i2c_repstart(void);

//! procedure writes one data byte to the I2C device with the corresponding address
void i2c_write_data (unsigned char address, unsigned char data);

//! procedure writes one data byte to the I2C device register with the corresponding address
void i2c_write_reg (unsigned char address, unsigned char reg, unsigned char data);

//! function reads two data bytes from the I2C bus
void i2c_read_two_data (unsigned char address, unsigned char *data_MSB, unsigned char *data_LSB); 

//! procedure reads two data bytes from the I2C device register with the corresponding address
void i2c_read_two_regs (unsigned char address, unsigned char reg, unsigned char *data_MSB, unsigned char *data_LSB);

#endif