/** 
 * @file  buffer.h It contains a buffer description structure and declarations of methods to work with him
 */


#ifndef BUFFER_H
#define BUFFER_H
 
    //structure/typdefs
    //!the cBuffer structure
    typedef struct struct_cBuffer
    {
        unsigned char *dataptr;         /// the physical memory address where the buffer is stored
        unsigned char size;             /// the allocated size of the buffer
        unsigned char datalength;       /// the length of the data currently in the buffer
        unsigned char dataindex;        /// the index into the buffer where the data starts
    }   cBuffer;
 
    // function prototypes
 
    //! initialize a buffer to start at a given address and have given size
     void  bufferInit(cBuffer* buffer, unsigned char *start, unsigned char size);
     
    //! get the first byte from the front of the buffer
    unsigned char   bufferGetFromFront(cBuffer* buffer);
    
    //! add a byte to the end of the RX buffer
    void RX_bufferAddToEnd(cBuffer* buffer, unsigned char data);
    
    
    
    //! dump (discard) the first numbytes from the front of the buffer
    //void bufferDumpFromFront(cBuffer* buffer, unsigned char numbytes);
    
    //! get a byte at the specified index in the buffer (kind of like array access)
    // ** note: this does not remove the byte that was read from the buffer
    //unsigned char   bufferGetAtIndex(cBuffer* buffer, unsigned char index);
    
    //! check if the buffer is full/not full (returns non-zero value if not full)
    // unsigned char   bufferIsNotFull(cBuffer* buffer);
    
    //! flush (clear) the contents of the buffer
    // void bufferFlush(cBuffer* buffer);
 
 #endif