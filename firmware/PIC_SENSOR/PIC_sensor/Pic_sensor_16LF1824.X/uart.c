/** 
 * @file  uart.c  It contains implementation of methods to work with hardware uart module 
 */

#include <pic16f1824.h>
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "uart.h"
#include "buffer.h"


 //!UART global variables
 //!flag variables
 volatile boolean   uartReadyTx = TRUE;         ///< uartReadyTx flag
 volatile boolean   uartBufferedTx;             ///< uartBufferedTx flag
 volatile boolean   CR_flag = FALSE;            ///< uart receive CR_flag
 volatile boolean   STX_flag = FALSE;           ///< uart receive STX_flag
 
 
 //!receive and transmit buffers
 cBuffer uartRxBuffer;                          ///< uart receive buffer
 cBuffer uartTxBuffer;                          ///< uart transmit buffer
 unsigned char uartRxOverflow;                  ///< receive overflow counter
 volatile unsigned char rx_byte_counter = 0;    ///< counter of bytes on received packet

 static char uartRxData[UART_RX_BUFFER_SIZE];   ///< receive buffer
 static char uartTxData[UART_TX_BUFFER_SIZE];   ///< transmit buffer
 


    
     
 /**
 procedure initialize a uart buffer to start at a given address and have given size
 */     
    void uartInitBuffers(void)
    {
        //! initialize the UART receive buffer
        bufferInit(&uartRxBuffer, uartRxData, UART_RX_BUFFER_SIZE);
        //! initialize the UART transmit buffer
        bufferInit(&uartTxBuffer, uartTxData, UART_TX_BUFFER_SIZE);     
    }
     

     
 /**
 initialize hardware uart module 
 */
    void uart_init()
    { 
        INTCONbits.GIE = 0;                         ///< global interrupt disable
        INTCONbits.PEIE = 0;                        ///< peripheral interrupt disable
        PIE1bits.TXIE = 0;                          ///< disable tx interrupts
       
      
        TRISCbits.TRISC4 = 0;                       ///< TX pin set as output
        TRISCbits.TRISC5 = 1;                       ///< RX pin set as input
      
        //!initialize the buffers
        uartInitBuffers();
    
        /**
        * Configure BRG
        * 9600 bps assuming 16MHz clock
        */
        TXSTAbits.BRGH = 1;                         ///< high baud rate
        BAUDCONbits.BRG16 = 0;                      ///< high baud rate
        TXSTAbits.TX9  = 0;                         ///< 8-bit transmission
        RCSTAbits.RX9 = 0;                          ///< 8-bit reception
        SPBRG = 103;                                ///< (16 MHz / (16 * 9600)) - 1 = 103

        /**
         *  Enable EUSART
         */
        RCSTAbits.CREN = 1;                         ///< continuous receive enable
        PIE1bits.RCIE = 1;                          ///< enable rx interrupts
        TXSTAbits.SYNC = 0;                         ///< asynchronous mode
        RCSTAbits.SPEN = 1;                         ///< enable the port
        TXSTAbits.TXEN = 0;                         ///< reset transmitter
        TXSTAbits.TXEN = 1;                         ///< enable transmitter
    
        //! initialize states
        uartReadyTx = TRUE;
        uartBufferedTx = FALSE;
        //! clear overflow count
        uartRxOverflow = 0;
         
        INTCONbits.GIE = 1;                         ///< global interrupt enable
        INTCONbits.PEIE = 1;                        ///< peripheral interrupt enable
        PIE1bits.TXIE = 0;                          ///< disable tx interrupts       
    }


  /**
  function get the first byte from the front of the RX buffer 
  @param rxData - a pointer to existing global variable
  */
    void uartReceiveByte(unsigned char* rxData)
    {
         //! make sure we have a receive buffer
         //! make sure we have data
         if(uartRxBuffer.datalength != uartRxBuffer.dataindex)
         {
             //! get byte from beginning of buffer
             *rxData = bufferGetFromFront(&uartRxBuffer);          
         } 
    }



 
 
  /*************************************** /////UartbufferAddToEnd////// *************************************************************/ 
  /**
  procedure add a byte to the end of the TX uart buffer
  @param buffer - a pointer to existing structure
  @param data - a data byte which added at the end of TX buffer
  */
    void uart_bufferAddToEnd(cBuffer* buffer, unsigned char data)
    {
         //! save data byte at end of buffer
        
         buffer->dataptr[ buffer->datalength] = data;
         //! increment the length
         buffer->datalength++;
         buffer->datalength &= TX_BUF_MASK;
         //! return success        
    }
    /*************************************** /////UartbufferAddToEnd////// *************************************************************/ 
 
 
 
 
 
 
 
 
    /*************************************** /////UartSendTxBuffer////// *************************************************************/ 
    
   /**
   procedure sends a full buffer via the uart without using interrupt control
   @param buffer - a pointer to existing structure of TX buffer
   */   
    void uartSendTxBuffer(cBuffer* buffer)
    {
        unsigned char data = 0;

        //! check to see if there's data in the buffer
        while (buffer->datalength != buffer->dataindex)
        {
            //! get the first character from buffer
            data = buffer->dataptr[buffer->dataindex];
            //! move index down and decrement length
            buffer->dataindex++;
            buffer->dataindex &= TX_BUF_MASK;
           
            //! wait for previous transmission to finish
            while(!TXIF);                                                       
            TXREG = data;         
        }       
    }
    /*************************************** /////UartSendTxBuffer////// *************************************************************/
  
  

 



   /***************************************UART SEND RECEIVE ONE BYTE*************************************************************/
   /**
   procedure looks for flag TXIF
   @return TXIF - a flag which indicate state of transmitter uart module
   */ 
    boolean uart_canSend()
    {
        return TXIF;
    }

   /**
   procedure sends a single byte over the uart
   @param data - a data byte which transmitted via uart 
   */
    void uart_send_byte(unsigned char data)
    {
        while(!TXIF);                                                           ///< wait for previous transmission to finish
        TXREG = data;
    }

   /**
   procedure looks for flag RCIF
   @return RCIF - a flag which indicate state of receiver uart module
   */ 
    boolean uart_canReceive()
    {
        return PIR1bits.RCIF;
    }
  
  
   /***************************************UART SEND RECEIVE ONE BYTE*************************************************************/




   /***************************************************ISR*************************************************************/
   /**
   procedure interrupt ISR of PIC16LF1824 (see datasheet for more details)
   */ 
    void interrupt ISR(void)
    {
        unsigned char temp_rx_isrbyte = 0; 
        
        if(RCIF)                                                                ///< If UART Rx Interrupt
        {
            if(RCSTAbits.FERR && !PORTCbits.RC5) {                              ///< Bootloader_MODE
                asm("RESET");
            }
            
            if(RCSTAbits.OERR) { ///< If over run error, then reset the receiver
                RCSTAbits.CREN = 0;
                RCSTAbits.CREN = 1;
            }
        
            temp_rx_isrbyte = RCREG;
            
            if ((temp_rx_isrbyte == 0x02) && (!CR_flag)) {                      ///< indicate start of packet 
                STX_flag = TRUE;
            }                                                                   
            
            if (temp_rx_isrbyte == 0x0D) {
                CR_flag = TRUE; 
                rx_byte_counter = 0; 
                STX_flag = FALSE; 
                RX_bufferAddToEnd(&uartRxBuffer, temp_rx_isrbyte);
            }   ///< indicate end of packet
            
            if((STX_flag) && (!CR_flag)) {
                RX_bufferAddToEnd(&uartRxBuffer, temp_rx_isrbyte);
                rx_byte_counter++;
                if(rx_byte_counter > 62) {
                    uartRxBuffer.dataindex = uartRxBuffer.datalength; 
                    rx_byte_counter = 0; 
                    STX_flag = FALSE; 
                }                  ///< 62 bytes on packet max!!!!! and clear uartRxBuffer        
            }
                             
        }   // end of receive interrupt 
    } // end of ISR

    /***************************************************ISR*************************************************************/

 
 
 
 
 
 
