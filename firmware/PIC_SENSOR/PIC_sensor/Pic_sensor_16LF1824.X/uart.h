/** 
 * @file  uart.h  It contains declarations of methods to work with uart  
 */

#ifndef UART_H
#define	UART_H

#include "mtypes.h"
#include "buffer.h"


 //! default baud rate
 #define UART_DEFAULT_BAUD_RATE  9600
 
 //! buffer memory allocation defines
 //! buffer sizes
 #ifndef UART_TX_BUFFER_SIZE
 #define UART_TX_BUFFER_SIZE     0x20                                           /// number of bytes for uart transmit buffer ( must be ~2^n)
 #define TX_BUF_MASK             (UART_TX_BUFFER_SIZE-1)
 #endif
 #ifndef UART_RX_BUFFER_SIZE
 #define UART_RX_BUFFER_SIZE     0x40                                           /// number of bytes for uart receive buffer  ( must be ~2^n)
 #define RX_BUF_MASK               (UART_RX_BUFFER_SIZE-1)
 #endif
     

 // functions
 //! initializes transmit and receive buffers
 //! called from uart_init()
 void uartInitBuffers(void);
 
 //! initializes uart
 void uart_init();
 
 //! sends a single byte over the uart
  void uart_send_byte(unsigned char data);
 
 /**
  * gets a single byte from the uart receive buffer
  * Function returns TRUE if data was available, FALSE if not.
  * Actual data is returned in variable pointed to by "data".
  * example usage:
  * char myReceivedByte;
  * uartReceiveByte( &myReceivedByte );
  */
 void uartReceiveByte(unsigned char* data);
 
 //! returns TRUE/FALSE if receive buffer is empty/not-empty
 unsigned char uartReceiveBufferIsEmpty(void);
 
 //! returns TRUE/FALSE if receive buffer is empty/not-empty
 unsigned char uartTxBufferIsEmpty(void);
 

 
 //! add byte to end of uart Tx buffer
 void uart_bufferAddToEnd(cBuffer* buffer, unsigned char data);
 
 //! sends a full buffer via the uart without using interrupt control
 void uartSendTxBuffer(cBuffer* buffer);

 //! sends a buffer of length nBytes via the uart using interrupt control
 unsigned char  uartSendBuffer(unsigned char *buffer, unsigned char nBytes);
 
 //! interrupt service handlers
 void uartTransmitService(void);
 
 //! look for flag TXIF
 boolean uart_canSend();
 
 //! look for flag RCIF
 boolean uart_canReceive();


#endif	/* UART_H */