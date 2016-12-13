/** 
 * @file  buffer.c It contains a buffer initialization structure and implementation of methods to work with him
 */ 

#include "buffer.h"
#include "uart.h"
 
 //! global variables
 //! initialization
 
 /**
 procedure initialize a buffer to start at a given address and have given size
 @param buffer - a pointer to initialize structure
 @param start - a pointer to the physical memory address where the buffer is stored
 @param size - the allocated size of the buffer
 */
 void bufferInit(cBuffer* buffer, unsigned char *start, unsigned char size)
 {
     //! set start pointer of the buffer
     buffer->dataptr = start;
     buffer->size = size;
     //! initialize index and length
     buffer->dataindex = 0;
     buffer->datalength = 0;
 }
 
 /**
 function get the first byte from the front of the RX buffer only, because using RX_BUF_MASK
 @param buffer - a pointer to existing structure
 */
 unsigned char  bufferGetFromFront(cBuffer* buffer)
 {
     unsigned char data = 0;
     
     //! check to see if there's data in the buffer
     if (buffer->datalength != buffer->dataindex)
     {
         //! get the first character from buffer
         data = buffer->dataptr[buffer->dataindex];
         //! move index down and decrement length
         buffer->dataindex++;
         buffer->dataindex &= RX_BUF_MASK;        
     }
     //! return
     return data;
 }
 

 /**
 procedure add a byte to the end of the RX buffer
 @param buffer - a pointer to existing structure
 @param data - a data byte which added at the end of RX buffer
 */
 void  RX_bufferAddToEnd(cBuffer* buffer, unsigned char data)
 {
         //! make sure the buffer has room
         //! save data byte at end of buffer
         buffer->dataptr[buffer->datalength] = data;
         //! increment the length
         buffer->datalength++;
         buffer->datalength &= RX_BUF_MASK;        
  }
 

 

