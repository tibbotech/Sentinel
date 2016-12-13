/** 
 * @file  hardw_i2c.c It contains implementation of methods to work with hardware I2C module 
 */

#include "hardw_i2c.h"
#include <xc.h>
#include <pic16f1824.h>

/**
 initialize hardware I2C module 
 */
void i2c_init_hardw ()
{
    TRISCbits.TRISC0 = 1;            ///< SCL pin set as input
    TRISCbits.TRISC1 = 1;            ///< SDA pin set as input
    ANSELCbits.ANSC0 = 0;            ///< Digital I/O
    ANSELCbits.ANSC1 = 0;            ///< Digital I/O
     
    WPUCbits.WPUC0 = 1;              ///< Weak pull-up
    WPUCbits.WPUC1 = 1;              ///< Weak pull-up
    
    SSP1CON1bits.SSPM=0x08;          ///< I2C Master mode, clock = Fosc/(4 * (SSP1ADD+1))
    SSPSTAT = 0x80;                  ///< Disable slew rate control
    SSP1ADD = 0x09;                  ///< 400 kHZ
    SSPCON2 = 0b000000000;           ///< Clear flags
  
    SSP1CON1bits.SSPEN=1;            ///< enable MSSP     
}

/**
 procedure sends one address byte to the I2C bus
 @param address - an address of the I2C device
 */
void i2c_send_address(unsigned char address)
{
    PIR1bits.SSP1IF=0;
    SSPBUF = address;
    //! wait for complete transmit
    while(SSP1STATbits.BF);     
}

/**
 procedure sends one data byte to the I2C bus
 @param data - a data byte which sends to the i2c bus
 */
void i2c_send_data(unsigned char data)
{
    PIR1bits.SSP1IF=0;
    SSPBUF = data;
    while(SSP1STATbits.BF); 
}

/**
 function reads one data byte from the I2C bus
 @return SSPBUF - a data byte which reads from the I2C bus 
 */
unsigned char i2c_read_data(void)
{
    PIR1bits.SSP1IF=0;
    SSPCON2bits.RCEN=1;
    while(SSP1CON2bits.RCEN);                                                   ///< Wait for the RCEN bit to go back low before we load the data buffer
    return (SSPBUF);
}

/**
procedure performs start of hardware I2C module
 */
void i2c_start(void)
{
    SSPCON2bits.SEN=1;
    while(SSP1CON2bits.SEN);                                                    ///< Wait for the SEN bit to go back low before we load the data buffer
    PIR1bits.SSP1IF=0;
}

/**
procedure performs stop of hardware I2C module
 */
void i2c_stop(void)
{
    PIR1bits.SSP1IF=0;
    SSPCON2bits.PEN=1;
    while(SSP1CON2bits.PEN);                                                    ///< Wait for the PEN bit to go back low before we load the data buffer
}


/**
procedure performs acknowledgment of receiving data from I2C bus
 */
void i2c_ack(void)
{ 
   SSPCON2bits.ACKDT=0;
   SSPCON2bits.ACKEN=1;
}

/**
procedure performs no acknowledgment of receiving data from I2C bus
 */
void i2c_nak(void)
{
    SSPCON2bits.ACKDT=1;
    SSPCON2bits.ACKEN=1;
}

/**
 procedure performs waiting for the I2C bus to go into Idle mode 
 */
void i2c_idle( void )
{
	while( ( SSPCON2 & 0x1F ) | SSP1STATbits.R_nW );
}

/**
 procedure performs repeating start condition
 */
void i2c_repstart( void )
{
	SSPCON2bits.RSEN = 1;                   /// Enable Repeated Start condition
	while( SSPCON2bits.RSEN );              /// Wait for condition to be sent
}

/**
procedure writes one data byte to the I2C device with the corresponding address
@param address - an address of the I2C device
@param data - a data byte which sends to the I2C bus
*/
void i2c_write_data (unsigned char address, unsigned char data)
{
    i2c_idle();
    i2c_start();
    i2c_send_address(address);               ///write address
    i2c_send_data(data);                     ///write data
    i2c_idle();
    i2c_stop();     
}

/**
procedure writes one data byte to the I2C device register with the corresponding address
@param address - an address of the I2C device
@param reg - an address of the I2C device register
@param data - a data byte which sends to the I2C bus
*/
void i2c_write_reg (unsigned char address, unsigned char reg, unsigned char data)
{
     i2c_idle();
     i2c_start();
     i2c_send_address(address);               ///< write address
     i2c_send_data(reg);                      ///< write reg
     i2c_idle();
     i2c_send_data(data);                     ///< write data
     i2c_idle();
     i2c_stop();  
}

/**
function reads two data bytes from the I2C bus
@param address - an address of the I2C device
@param *data_MSB - a pointer to most significant data byte which reads from the I2C bus
@param *data_LSB - a pointer to least significant data byte which reads from the I2C bus
*/
void i2c_read_two_data (unsigned char address, unsigned char *data_MSB, unsigned char *data_LSB) 
{
      i2c_idle();
      i2c_start();
      i2c_send_address(address);              ///< write address 
      *data_MSB = i2c_read_data();
      i2c_idle();
      i2c_ack();
      i2c_idle();
      *data_LSB = i2c_read_data();
      i2c_idle();
      i2c_nak();
      i2c_idle();
      i2c_stop();
}

/**
function reads two data bytes from the I2C device registers
@param address - an address of the I2C device
@param reg - an address of the I2C device register 
@param *data_MSB - a pointer to most significant data byte which reads from the I2C device register
@param *data_LSB - a pointer to least significant data byte which reads from the I2C device register
*/
void i2c_read_two_regs (unsigned char address, unsigned char reg, unsigned char *data_MSB, unsigned char *data_LSB)
{
      i2c_idle();
      i2c_start();
      i2c_send_address(address);                ///< write address 
      i2c_send_data(reg);
      i2c_idle();
      i2c_start();
      i2c_send_address(address | 0x01);
      *data_MSB = i2c_read_data();
      i2c_idle();
      i2c_ack();
      i2c_idle();
      *data_LSB = i2c_read_data();
      i2c_idle();
      i2c_nak();
      i2c_idle();
      i2c_stop(); 
}










